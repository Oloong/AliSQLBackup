/* Copyright (c) 2011, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   51 Franklin Street, Suite 500, Boston, MA 02110-1335 USA */

#include "zgroups.h"


#ifdef HAVE_UGID


const size_t Uuid::TEXT_LENGTH;
const size_t Uuid::BYTE_LENGTH;
const size_t Uuid::BIT_LENGTH;
const int Uuid::bytes_per_section[Uuid::NUMBER_OF_SECTIONS]=
{ 4, 2, 2, 2, 6 };
const int Uuid::hex_to_byte[]=
{
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
  -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};


enum_return_status Uuid::parse(const char *s)
{
  DBUG_ENTER("Uuid::parse");
  unsigned char *u= bytes;
  unsigned char *ss= (unsigned char *)s;
  for (int i= 0; i < NUMBER_OF_SECTIONS; i++)
  {
    if (i > 0)
    {
      if (*ss != '-')
        RETURN_UNREPORTED_ERROR;
      ss++;
    }
    for (int j= 0; j < bytes_per_section[i]; j++)
    {
      int hi= hex_to_byte[*ss];
      if (hi == -1)
        RETURN_UNREPORTED_ERROR;
      ss++;
      int lo= hex_to_byte[*ss];
      if (lo == -1)
        RETURN_UNREPORTED_ERROR;
      ss++;
      *u= (hi << 4) + lo;
      u++;
    }
  }
  RETURN_OK;
}


bool Uuid::is_valid(const char *s)
{
  DBUG_ENTER("Uuid::is_valid");
  const unsigned char *ss= (const unsigned char *)s;
  for (int i= 0; i < NUMBER_OF_SECTIONS; i++)
  {
    if (i > 0)
    {
      if (*ss != '-')
        DBUG_RETURN(false);
      ss++;
    }        
    for (int j= 0; j < bytes_per_section[i]; j++)
    {
      if (hex_to_byte[*ss] == -1)
        DBUG_RETURN(false);
      ss++;
      if (hex_to_byte[*ss] == -1)
        DBUG_RETURN(false);
      ss++;
    }
  }
  DBUG_RETURN(true);
}


int Uuid::to_string(char *buf) const
{
  DBUG_ENTER("Uuid::to_string");
  static const char byte_to_hex[]= "0123456789ABCDEF";
  const unsigned char *u= bytes;
  for (int i= 0; i < NUMBER_OF_SECTIONS; i++)
  {
    if (i > 0)
    {
      *buf= '-';
      buf++;
    }
    for (int j= 0; j < bytes_per_section[i]; j++)
    {
      int byte= *u;
      *buf= byte_to_hex[byte >> 4];
      buf++;
      *buf= byte_to_hex[byte & 0xf];
      buf++;
      u++;
    }
  }
  *buf= '\0';
  DBUG_RETURN(TEXT_LENGTH);
}


enum_return_status Uuid::write(File fd, myf my_flags) const
{
  DBUG_ENTER("Uuid::write(File, myf)");
  if (my_write(fd, bytes, Uuid::BYTE_LENGTH, my_flags) != Uuid::BYTE_LENGTH)
  {
    if ((my_flags & MY_WME) != 0)
      RETURN_REPORTED_ERROR;
    else
      RETURN_UNREPORTED_ERROR;
  }
  RETURN_OK;
}


enum_read_status Uuid::read(File fd, myf my_flags)
{
  DBUG_ENTER("Uuid::read(File, myf)");
  size_t read_bytes= my_read(fd, bytes, Uuid::BYTE_LENGTH, my_flags);
  if (read_bytes == 0)
    DBUG_RETURN(READ_EOF);
  if (read_bytes < Uuid::BYTE_LENGTH)
    DBUG_RETURN(READ_TRUNCATED);
  if (read_bytes == MY_FILE_ERROR)
    DBUG_RETURN(READ_ERROR_IO);
  DBUG_RETURN(READ_OK);
}


#endif /* HAVE_UGID */