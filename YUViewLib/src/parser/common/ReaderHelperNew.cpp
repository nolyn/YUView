/*  This file is part of YUView - The YUV player with advanced analytics toolset
 *   <https://github.com/IENT/YUView>
 *   Copyright (C) 2015  Institut f�r Nachrichtentechnik, RWTH Aachen University, GERMANY
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   In addition, as a special exception, the copyright holders give
 *   permission to link the code of portions of this program with the
 *   OpenSSL library under certain conditions as described in each
 *   individual source file, and distribute linked combinations including
 *   the two.
 *
 *   You must obey the GNU General Public License in all respects for all
 *   of the code used other than OpenSSL. If you modify file(s) with this
 *   exception, you may extend this exception to your version of the
 *   file(s), but you are not obligated to do so. If you do not wish to do
 *   so, delete this exception statement from your version. If you delete
 *   this exception statement from all source files in the program, then
 *   also delete it here.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ReaderHelperNew.h"

#include <sstream>

namespace parser
{

namespace
{

template <typename T> std::string formatCoding(const std::string formatName, T value)
{
  std::ostringstream stringStream;
  stringStream << formatName;
  if (formatName != "u(1)")
    stringStream << " -> u(" << value << ")";
  return stringStream.str();
}

std::string rangeCheckErrorMessage(const ReaderHelperNew::Options &options, int64_t value)
{
  if (options.checkMinMaxSigned &&
      (value < options.checkMinMaxSigned->first || value > options.checkMinMaxSigned->second))
  {
    return "Value should be in the range of " + std::to_string(options.checkMinMaxSigned->first) +
           " to " + std::to_string(options.checkMinMaxSigned->second) + " inclusive.";
  }
  return {};
}

std::string rangeCheckErrorMessage(const ReaderHelperNew::Options &options, uint64_t value)
{
  if (options.checkMinMaxUnsigned &&
      (value < options.checkMinMaxUnsigned->first || value > options.checkMinMaxUnsigned->second))
  {
    return "Value should be in the range of " + std::to_string(options.checkMinMaxUnsigned->first) +
           " to " + std::to_string(options.checkMinMaxUnsigned->second) + " inclusive.";
  }
  return {};
}

void rangeCheckThrow(const ReaderHelperNew::Options &options, int64_t value)
{
  auto errorString = rangeCheckErrorMessage(options, value);
  if (!errorString.empty())
    throw std::logic_error(errorString);
}

void rangeCheckThrow(const ReaderHelperNew::Options &options, uint64_t value)
{
  auto errorString = rangeCheckErrorMessage(options, value);
  if (!errorString.empty())
    throw std::logic_error(errorString);
}

} // namespace

ByteVector ReaderHelperNew::convertBeginningToByteArray(QByteArray data)
{
  ByteVector ret;
  const auto maxLength = 2000u;
  const auto length    = std::min(unsigned(data.size()), maxLength);
  for (auto i = 0u; i < length; i++)
  {
    ret.push_back(data.at(i));
  }
  return ret;
}

ReaderHelperNew::ReaderHelperNew(SubByteReaderNew &reader,
                                 TreeItem *        item,
                                 std::string       new_sub_item_name)
{
  this->reader = reader;
  if (item)
  {
    if (new_sub_item_name.empty())
      this->currentTreeLevel = item;
    else
      this->currentTreeLevel = new TreeItem(item, new_sub_item_name);
  }
  this->itemHierarchy.push(this->currentTreeLevel);
}

ReaderHelperNew::ReaderHelperNew(const ByteVector &inArr,
                                 TreeItem *        item,
                                 std::string       new_sub_item_name,
                                 size_t            inOffset)
{
  this->reader = SubByteReaderNew(inArr, inOffset);
  if (item)
  {
    if (new_sub_item_name.empty())
      this->currentTreeLevel = item;
    else
      this->currentTreeLevel = new TreeItem(item, new_sub_item_name);
  }
  this->itemHierarchy.push(this->currentTreeLevel);
}

void ReaderHelperNew::addLogSubLevel(const std::string name)
{
  assert(!name.empty());
  if (itemHierarchy.top() == nullptr)
    return;
  this->currentTreeLevel = new TreeItem(this->itemHierarchy.top(), name);
  this->itemHierarchy.push(this->currentTreeLevel);
}

void ReaderHelperNew::removeLogSubLevel()
{
  if (itemHierarchy.size() <= 1)
    // Don't remove the root
    return;
  this->itemHierarchy.pop();
  this->currentTreeLevel = this->itemHierarchy.top();
}

uint64_t
ReaderHelperNew::readBits(const std::string &symbolName, int numBits, const Options &options)
{
  try
  {
    auto [value, code] = this->reader.readBits(numBits);
    this->logRead("u(v)", symbolName, options, int64_t(value), code);
    rangeCheckThrow(options, value);
    return value;
  }
  catch (const std::exception &ex)
  {
    this->logExceptionAndThrowError(ex, std::to_string(numBits) + " bit symbol " + symbolName);
  }
}

bool ReaderHelperNew::readFlag(const std::string &symbolName, const Options &options)
{
  try
  {
    auto [value, code] = this->reader.readBits(1);
    this->logRead("u(1)", symbolName, options, int64_t(value), code);
    rangeCheckThrow(options, value);
    return (value != 0);
  }
  catch (const std::exception &ex)
  {
    this->logExceptionAndThrowError(ex, "flag " + symbolName);
  }
}

uint64_t ReaderHelperNew::readUEV(const std::string &symbolName, const Options &options)
{
  try
  {
    auto [value, code] = this->reader.readUE_V();
    this->logRead("ue(v)", symbolName, options, int64_t(value), code);
    rangeCheckThrow(options, value);
    return value;
  }
  catch (const std::exception &ex)
  {
    this->logExceptionAndThrowError(ex, "UEV symbol " + symbolName);
  }
}

int64_t ReaderHelperNew::readSEV(const std::string &symbolName, const Options &options)
{
  try
  {
    auto [value, code] = this->reader.readSE_V();
    this->logRead("se(v)", symbolName, options, value, code);
    rangeCheckThrow(options, value);
    return value;
  }
  catch (const std::exception &ex)
  {
    this->logExceptionAndThrowError(ex, "SEV symbol " + symbolName);
  }
}

void ReaderHelperNew::logRead(const std::string &formatName,
                              const std::string &symbolName,
                              const Options &    options,
                              int64_t            value,
                              const std::string &code)
{
  if (this->currentTreeLevel)
  {
    std::string meaning = options.meaningString;
    if (options.meaningMap.count(value) > 0)
      meaning = options.meaningMap.at(value);

    new TreeItem(currentTreeLevel,
                 symbolName,
                 std::to_string(value),
                 formatCoding(formatName, code.size()),
                 code,
                 meaning);
  }
}

void ReaderHelperNew::logExceptionAndThrowError(const std::exception &ex, const std::string &when)
{
  if (this->currentTreeLevel)
  {
    auto errorMessage = "Reading error " + std::string(ex.what());
    auto item         = new TreeItem(currentTreeLevel, "Error", "", "", "", errorMessage);
    item->setError();
  }
  throw std::logic_error("Error reading " + when);
}

} // namespace parser