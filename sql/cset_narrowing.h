/*
   Copyright (c) 2021, MariaDB Corporation.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335 USA */


/*
  This file is only to be included from sql_select.h
*/

/*
  A singleton class to provide "utf8mb3_from_mb4.charset()".

  This is a variant of utf8mb3_general_ci that one can use when they have data
  in MB4 and want to make index lookup keys in MB3.
*/
extern
class Charset_utf8narrow
{
  struct my_charset_handler_st cset_handler;
  struct charset_info_st cset;
public:
  Charset_utf8narrow() :
    cset_handler(*my_charset_utf8mb3_general_ci.cset),
    cset(my_charset_utf8mb3_general_ci) /* Copy the CHARSET_INFO structure */
  {
    /* Insert our function wc_mb */
    cset_handler.wc_mb= my_wc_mb_utf8mb4_bmp_only;
    cset.cset=&cset_handler;

    /* Charsets are compared by their name, so assign a different name */
    cset.cs_name.str ="utf8_mb4_to_mb3";
    cset.cs_name.length= strlen(cset.cs_name.str);
  }

  CHARSET_INFO *charset() { return &cset; }

} utf8mb3_from_mb4;


/*
  A RAII class to temporary change a field in utf8mb3_general_ci to allow
  correct lookup key construction from data in utf8mb4_general_ci

  Intended usage:

    // can do this in advance:
    bool do_narrowing=
      Utf8_narrow_RAII::should_do_narrowing(field, value_cset);
    ...

    // Now, change if necessary:
    {
      Utf8_narrow_RAII(field, do_narrowing);
      // write to 'field' here
    }
*/

class Utf8_narrow_RAII
{
  Field *field;
  DTCollation save_collation;

public:
  static bool should_do_narrowing(const Field *field, CHARSET_INFO *value_cset)
  {
    CHARSET_INFO *to_cset= field->charset();
    return (to_cset    == &my_charset_utf8mb3_general_ci &&
            value_cset == &my_charset_utf8mb4_general_ci);
  }


  Utf8_narrow_RAII(Field *field_arg, bool is_applicable)
  {
    if (is_applicable)
    {
      DTCollation mb3_from_mb4= utf8mb3_from_mb4.charset();
      field= field_arg;
      save_collation= field->dtcollation();
      field->change_charset(mb3_from_mb4);
    }
    else
      field= NULL;
  }

  ~Utf8_narrow_RAII()
  {
    if (field)
     field->change_charset(save_collation);
  }
};

