// 2000-08-31 Benjamin Kosnik <bkoz@redhat.com>

// Copyright (C) 2000 Free Software Foundation
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING.  If not, write to the Free
// Software Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307,
// USA.

// 22.1.1.1.2 - class locale::facet [lib.locale.facet]

#include <locale>
#include <iterator>
#include <debug_assert.h>

// 1 a class if a facet if it is publicly derived from another facet
class gnu_input_iterator: public std::iterator<std::input_iterator_tag, char>
{
  value_type it;
public:
  gnu_input_iterator(value_type orig): it(orig) { }

  value_type 
  operator*() const { return it; }

  reference
  operator++(){ return ++it; }

  reference
  operator++(int){ ++it; return it; }
};

bool
operator==(const gnu_input_iterator& lhs, const gnu_input_iterator& rhs)
{ return true; }

bool
operator!=(const gnu_input_iterator& lhs, const gnu_input_iterator& rhs)
{ return true; }

class gnu_output_iterator: public std::iterator<std::output_iterator_tag, char>
{ 
  value_type it;
public:
  gnu_output_iterator(value_type orig): it(orig) { }

  value_type
  operator*(){ return it; }

  gnu_output_iterator&
  operator=(value_type obj){ it = obj; return *this; }

  reference 
  operator++(){ return ++it; }

  reference 
  operator++(int){ ++it; return it; }

};

class gnu_collate: public std::collate<char> { }; 
class gnu_ctype: public std::ctype<char> { }; 
class gnu_codecvt: public std::codecvt<char, char, mbstate_t> { }; 
class gnu_moneypunct: public std::moneypunct<char> { }; 
class gnu_moneypunct_true: public std::moneypunct<char, true> { }; 
class gnu_money_get: public std::money_get<char> { }; 
class gnu_money_put: public std::money_put<char> { }; 
class gnu_numpunct: public std::numpunct<char> { }; 
class gnu_num_get: public std::num_get<char> { }; 
class gnu_num_put: public std::num_put<char> { }; 
class gnu_time_get: public std::time_get<char> { }; 
class gnu_time_put: public std::time_put<char> { }; 
class gnu_messages: public std::messages<char> { }; 

class gnu_collate_byname: public std::collate_byname<char> 
{
public:
  explicit
  gnu_collate_byname(const char* c, size_t refs = 0)
  : std::collate_byname<char>(c, refs) { }
}; 

class gnu_ctype_byname: public std::ctype_byname<char> 
{
public:
  explicit
  gnu_ctype_byname(const char* c, size_t refs = 0)
  : std::ctype_byname<char>(c, refs) { }
}; 

class gnu_moneypunct_byname_true: public std::moneypunct_byname<char, true> 
{
public:
  explicit
  gnu_moneypunct_byname_true(const char* c, size_t refs = 0)
  : std::moneypunct_byname<char, true>(c, refs) { }
}; 

class gnu_moneypunct_byname_false: public std::moneypunct_byname<char, false> 
{
public:
  explicit
  gnu_moneypunct_byname_false(const char* c, size_t refs = 0)
  : std::moneypunct_byname<char, false>(c, refs) { }
}; 


class gnu_money_get_in: public std::money_get<char, gnu_input_iterator>
{
public:
  explicit
  gnu_money_get_in(size_t refs = 0)
  : std::money_get<char, gnu_input_iterator>(refs) { }
};

class gnu_money_put_out: public std::money_put<char, gnu_output_iterator>
{
public:
  explicit
  gnu_money_put_out(size_t refs = 0)
  : std::money_put<char, gnu_output_iterator>(refs) { }
};

class gnu_numpunct_byname: public std::numpunct_byname<char>
{
public:
  explicit
  gnu_numpunct_byname(const char* c, size_t refs = 0)
  : std::numpunct_byname<char>(c, refs) { }
};

class gnu_num_get_in: public std::num_get<char, gnu_input_iterator> 
{
public:
  explicit
  gnu_num_get_in(size_t refs = 0)
  : std::num_get<char, gnu_input_iterator>(refs) { }
};

class gnu_num_put_out: public std::num_put<char, gnu_output_iterator> 
{
public:
  explicit
  gnu_num_put_out(size_t refs = 0)
  : std::num_put<char, gnu_output_iterator>(refs) { }
};

class gnu_time_get_byname: public std::time_get_byname<char>
{
public:
  explicit
  gnu_time_get_byname(const char* c, size_t refs = 0)
  : std::time_get_byname<char>(c, refs) { }
};

class gnu_time_get_in: public std::time_get<char, gnu_input_iterator>
{
public:
  explicit
  gnu_time_get_in(size_t refs = 0)
  : std::time_get<char, gnu_input_iterator>(refs) { }
};

class gnu_time_put_byname: public std::time_put_byname<char> 
{
public:
  explicit
  gnu_time_put_byname(const char* c, size_t refs = 0)
  : std::time_put_byname<char>(c, refs) { }
};

class gnu_time_put_out: public std::time_put<char, gnu_output_iterator> 
{
public:
  explicit
  gnu_time_put_out(size_t refs = 0)
  : std::time_put<char, gnu_output_iterator>(refs) { }
};

class gnu_messages_byname: public std::messages_byname<char> 
{
public:
  explicit
  gnu_messages_byname(const char* c, size_t refs = 0)
  : std::messages_byname<char>(c, refs) { }
};


// 2 or if it is a class deerived from locale:;facet and containing a
// publicly-accessible declaration as follows:
class gnu_facet: public std::locale::facet
{
public:
  static std::locale::id id;
};

std::locale::id gnu_facet::id;

void test01()
{
  // 1
  gnu_collate 			obj01;
  gnu_ctype 			obj02;
  gnu_codecvt 			obj03;
  gnu_moneypunct		obj04;
  gnu_moneypunct_true		obj05;
  gnu_money_get			obj06;
  gnu_money_put			obj07;
  gnu_numpunct			obj08;
  gnu_num_get			obj09;
  gnu_num_put			obj10;
  gnu_time_get			obj11;
  gnu_time_put			obj12;
  gnu_messages			obj13;
  gnu_time_put_out		obj14(0);
  gnu_time_put_byname		obj15("gnu_message_byname", 0);
  gnu_time_get_in		obj16(0);
  gnu_time_get_byname		obj17("gnu_message_byname", 0);
  // gnu_num_put_out		obj18(0);
  // gnu_num_get_in		obj19(0);
  gnu_numpunct_byname		obj20("gnu_message_byname", 0);
  gnu_money_put_out		obj21(0);
  gnu_money_get_in		obj22(0);
  gnu_moneypunct_byname_false	obj23("gnu_message_byname", 0);
  gnu_moneypunct_byname_true	obj24("gnu_message_byname", 0);
  gnu_ctype_byname		obj25("gnu_message_byname", 0);
  gnu_collate_byname		obj26("gnu_message_byname", 0);
  gnu_messages_byname		obj27("gnu_message_byname", 0);

  // 2
  gnu_facet 			obj28;
}

int main ()
{
  test01();

  return 0;
}








