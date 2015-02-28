/*
 * Copyright 2012-2015 Falltergeist Developers.
 *
 * This file is part of Falltergeist.
 *
 * Falltergeist is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Falltergeist is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Falltergeist.  If not, see <http://www.gnu.org/licenses/>.
 */

// C++ standard includes
#include <string.h> // for memcpy
#include <algorithm>

// libfalltergeist includes
#include "../Dat/Entry.h"
#include "../Dat/File.h"
#include "../Dat/Item.h"

// Third party includes
#include "zlib.h"

namespace libfalltergeist
{
namespace Dat
{

Item::Item(std::ifstream * stream)
{
    _stream = stream;
}


Item::Item(std::shared_ptr<Entry> datFileEntry)
{
    _datFileEntry = datFileEntry;
    setFilename(datFileEntry->filename());
}

void Item::_initialize()
{
    if (_initialized) return;
    _initialized = true;

    if (_stream != 0)
    {
        _stream->seekg(0, std::ios::end);
        _size = _stream->tellg();
        _stream->seekg(0, std::ios::beg);

        _buffer = new uint8_t[_size];
        _stream->read((char*)_buffer, _size);
        _stream->close();
        setg((char*)_buffer, (char*)_buffer, (char*)_buffer + _size);
        return;
    }

    if (_datFileEntry != 0)
    {

        _buffer = new uint8_t[_datFileEntry->unpackedSize()];
        _size = _datFileEntry->unpackedSize();

        auto datFile = _datFileEntry->datFile();
        unsigned int oldPos = datFile->position();
        datFile->setPosition(_datFileEntry->dataOffset());

        if (_datFileEntry->compressed())
        {
            char * packedData = new char[_datFileEntry->packedSize()]();
            datFile->readBytes(packedData, _datFileEntry->packedSize());

            // unpacking
            z_stream zStream;
            zStream.total_in  = zStream.avail_in  = _datFileEntry->packedSize();
            zStream.avail_in = _datFileEntry->packedSize();
            zStream.next_in  = (unsigned char *)packedData;
            zStream.total_out = zStream.avail_out = _size;
            zStream.next_out = (unsigned char *)_buffer;
            zStream.zalloc = Z_NULL;
            zStream.zfree = Z_NULL;
            zStream.opaque = Z_NULL;
            inflateInit( &zStream );            // zlib function
            inflate( &zStream, Z_FINISH );      // zlib function
            inflateEnd( &zStream );             // zlib function

            delete [] packedData;
        }
        else
        {
            datFile->readBytes((char*)_buffer, _size);
        }

        datFile->setPosition(oldPos);
        setg((char*)_buffer, (char*)_buffer, (char*)_buffer + _size);
        return;
    }
}


Item::~Item()
{
    delete [] _buffer;
}

unsigned int Item::size()
{
    _initialize();
    return _size;
}

std::streambuf::int_type Item::underflow()
{
    _initialize();
    if (gptr() == egptr())
    {
        return traits_type::eof();
    }
    return traits_type::to_int_type(*gptr());
}

Item* Item::setFilename(const std::string filename)
{
    _filename = filename;

    // convert to lowercase and replace slashes
    std::replace(_filename.begin(),_filename.end(),'\\','/');
    std::transform(_filename.begin(),_filename.end(),_filename.begin(), ::tolower);
    return this;
}

std::string Item::filename()
{
    return _filename;
}

Item* Item::setPosition(unsigned int pos)
{
    _initialize();
    setg((char*)_buffer, (char*)_buffer + pos, (char*)_buffer + _size);
    return this;
}

unsigned int Item::position()
{
    _initialize();
    return gptr() - eback();
}

Item* Item::skipBytes(unsigned int numberOfBytes)
{
    _initialize();
    setg((char*)_buffer, gptr() + numberOfBytes, (char*)_buffer + _size);
    return this;
}

Item* Item::readBytes(char * destination, unsigned int size)
{
    _initialize();
    sgetn(destination, size);
    return this;
}


Item& Item::operator>>(uint32_t &value)
{
    _initialize();
    char * buff = reinterpret_cast<char *>(&value);
    sgetn(buff, sizeof(value));
    if (endianness() == ENDIANNESS_BIG) std::reverse(buff, buff + sizeof(value));
    return *this;
}

Item& Item::operator>>(int32_t &value)
{
    return *this >> (uint32_t&) value;
}

Item& Item::operator>>(uint16_t &value)
{
    _initialize();
    char * buff = reinterpret_cast<char *>(&value);
    sgetn(buff, sizeof(value));
    if (endianness() == ENDIANNESS_BIG) std::reverse(buff, buff + sizeof(value));
    return *this;
}

Item& Item::operator>>(int16_t &value)
{
    return *this >> (uint16_t&) value;
}

Item& Item::operator>>(uint8_t &value)
{
    _initialize();
    sgetn(reinterpret_cast<char *>(&value), sizeof(value));
    return *this;
}

Item& Item::operator>>(int8_t &value)
{
    return *this >> (uint8_t&) value;
}

Endianness Item::endianness()
{
    return _endianness;
}

void Item::setEndianness(Endianness value)
{
    _endianness = value;
}

unsigned int Item::bytesRemains()
{
    return size() - position();
}

}
}