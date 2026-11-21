/**
 *  @file json.cpp
 *  @section License
 *
 *      Copyright (C) 2013 Ssss
 *      Copyright (C) 2023 Saif Kandil
 *
 *      This file is a part of the ENIGMA Development Environment.
 *
 *      This program is free software: you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation, either version 3 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#include "Widget_Systems/widgets_mandatory.h"
#include "json.h"
#include <cstdio>
#include <cmath>
#include <iostream>
#include <sstream>
#include <cstddef>
#include <vector>
#include <utility>
#include "../DataStructures/include.h"
#include "libjson.h"

using std::string;

namespace enigma_user
{
///auxiliary function
       static void DestroyJSONChildren(
           const std::vector<std::pair<int, bool>>& children)
       {
               for (const auto& child : children)
               {
                       if (child.second)
                               ds_list_destroy(child.first);
                       else
                               ds_map_destroy(child.first);
               }
       }

	constexpr unsigned JSON_MAX_NESTING_DEPTH = 256;
	
       constexpr std::size_t JSON_MAX_INPUT_SIZE = 16 * 1024 * 1024;
       
       static bool json_check_nesting_depth(const std::string& data)
       {
               unsigned depth = 0;
               bool in_string = false;
               bool escaped = false;

               for (const char c : data)
               {
                       if (in_string)
                       {
                               if (escaped)
                                       escaped = false;
                               else if (c == '\\')
                                       escaped = true;
                               else if (c == '"')
                                       in_string = false;

                               continue;
                       }

                       if (c == '"')
                               in_string = true;
                       else if (c == '{' || c == '[')
                       {
                               if (++depth > JSON_MAX_NESTING_DEPTH)
                                       return false;
                       }
                       else if (c == '}' || c == ']')
                       {
                               if (depth > 0)
                                       --depth;
                       }
               }

               return true;
       }

       static bool IsJSONInputSafe(const std::string& data)
       {
               if (data.size() > JSON_MAX_INPUT_SIZE)
                       return false;

               unsigned depth = 0;
               bool in_string = false;
               bool escaped = false;

               for (const char c : data)
               {
                       if (in_string)
                       {
                               if (escaped)
                                       escaped = false;
                               else if (c == '\\')
                                       escaped = true;
                               else if (c == '"')
                                       in_string = false;

                               continue;
                       }

                       if (c == '"')
                               in_string = true;
                       else if (c == '{' || c == '[')
                       {
                               if (++depth > JSON_MAX_NESTING_DEPTH)
                                       return false;
                       }
                       else if (c == '}' || c == ']')
                       {
                               if (depth > 0)
                                       --depth;
                       }
               }

               return true;
       }	

       int RecursiveDSList(const Json::Value& root, unsigned depth);

       int RecursiveDSMap(const Json::Value& root, unsigned depth)
	{
        if (depth >= JSON_MAX_NESTING_DEPTH)
        {
        return -1;        
        }
        int jsonObject=ds_map_create();
        std::vector<std::pair<int, bool>> children;

		Json::Value::Members m=root.getMemberNames();
		for (Json::Value::Members::iterator it = m.begin() ; it != m.end(); ++it)
		{
			if (root[*it].isNull())
			{
				ds_map_add(jsonObject,*it,0);
			}
			else if (root[*it].isBool())
			{
				ds_map_add(jsonObject,*it,root[*it].asInt());
			}
			else if (root[*it].isInt())
			{
				ds_map_add(jsonObject,*it,root[*it].asInt());
			}
			else if (root[*it].isUInt())
			{
				ds_map_add(jsonObject,*it,root[*it].asUInt());
			}
			else if (root[*it].isIntegral())
			{
				ds_map_add(jsonObject,*it,root[*it].asDouble());
			}
			else if (root[*it].isDouble())
			{
				ds_map_add(jsonObject,*it,root[*it].asDouble());
			}
			else if (root[*it].isNumeric())
			{
				ds_map_add(jsonObject,*it,root[*it].asDouble());
			}
			else if (root[*it].isString())
			{
				ds_map_add(jsonObject,*it,root[*it].asString());
			}
			else if (root[*it].isArray())
			{
				int dsmap=RecursiveDSList(root[*it], depth + 1);
				if (dsmap==-1)
				{
					DestroyJSONChildren(children);
					ds_map_destroy(jsonObject);
					return -1;
				}
				children.emplace_back(dsmap, true);
				ds_map_add(jsonObject,*it,dsmap);
			}
			else if (root[*it].isObject())
			{
				int dsmap=RecursiveDSMap(root[*it], depth + 1);
				if (dsmap==-1)
				{
					DestroyJSONChildren(children);
					ds_map_destroy(jsonObject);
					return -1;
				}
				children.emplace_back(dsmap, false);
				ds_map_add(jsonObject,*it,dsmap);
			}
			else
			{
				DestroyJSONChildren(children);
				ds_map_destroy(jsonObject);
				return -1;
			}
		}
		return jsonObject;
	}
	
	int RecursiveDSList(const Json::Value& root, unsigned depth)
	{

        if (depth >= JSON_MAX_NESTING_DEPTH)
        {
        return -1;
        }
        int jsonArray=ds_list_create();
        std::vector<std::pair<int, bool>> children;
        
		for (Json::ArrayIndex i = 0; i < root.size(); ++i)
		{
			if (root[i].isNull())
			{
				ds_list_add(jsonArray,(enigma::varargs(), 0));
			}
			else if (root[i].isBool())
			{
				ds_list_add(jsonArray,(enigma::varargs(), root[i].asInt()));
			}
			else if (root[i].isInt())
			{
				ds_list_add(jsonArray,(enigma::varargs(), root[i].asInt()));
			}
			else if (root[i].isUInt())
			{
				ds_list_add(jsonArray,(enigma::varargs(), root[i].asUInt()));
			}
			else if (root[i].isIntegral())
			{
				ds_list_add(jsonArray,(enigma::varargs(), root[i].asDouble()));
			}
			else if (root[i].isDouble())
			{
				ds_list_add(jsonArray,(enigma::varargs(), root[i].asDouble()));
			}
			else if (root[i].isNumeric())
			{
				ds_list_add(jsonArray,(enigma::varargs(), root[i].asDouble()));
			}
			else if (root[i].isString())
			{
				ds_list_add(jsonArray,(enigma::varargs(), root[i].asString()));
			}
			else if (root[i].isArray())
			{
				int dsmap=RecursiveDSList(root[i], depth + 1);
				if (dsmap==-1)
				{
					DestroyJSONChildren(children);
					ds_list_destroy(jsonArray);
					return -1;
				}
				children.emplace_back(dsmap, true);
				ds_list_add(jsonArray,(enigma::varargs(), dsmap));
			}
			else if (root[i].isObject())
			{
				int dsmap=RecursiveDSMap(root[i], depth + 1);
				if (dsmap==-1)
				{
					DestroyJSONChildren(children);
					ds_list_destroy(jsonArray);
					return -1;
				}
				children.emplace_back(dsmap, false);
				ds_list_add(jsonArray,(enigma::varargs(), dsmap));
			}
			else
			{
				DestroyJSONChildren(children);
				ds_list_destroy(jsonArray);
				return -1;
			}
		}
		return jsonArray;
	}
	
	variant json_decode(string data)
	{
		
               if (!IsJSONInputSafe(data))
               {
                       DEBUG_MESSAGE(
                               "JSON input exceeds the allowed size or nesting depth.",
                               MESSAGE_TYPE::M_ERROR
                       );
                       return -1;
               }
               
               if (!json_check_nesting_depth(data))
               {
                       DEBUG_MESSAGE(
                               "JSON nesting depth exceeds the maximum allowed depth.",
                               MESSAGE_TYPE::M_ERROR                      );
                       return -1;
               }

		Json::Value root;
		Json::Reader reader;
		const bool parsingSuccessful = reader.parse(data, root);
		if ( !parsingSuccessful )
		{
			DEBUG_MESSAGE("Failed to parse JSON" + reader.getFormattedErrorMessages(), MESSAGE_TYPE::M_ERROR);
			return -1;
		}

		if (root.isObject())
		{
			return RecursiveDSMap(root, 0);
		}
		else if (root.isArray())
		{
			return RecursiveDSList(root, 0);
		}
		else
		{
			DEBUG_MESSAGE("Invalid JSON. The root must be a JSON object or array.", MESSAGE_TYPE::M_ERROR);
			return -1;
		}
	}
static std::string json_encode_string(const std::string &value)
{
  Json::FastWriter writer;
  std::string encoded = writer.write(Json::Value(value));

  if (!encoded.empty() && encoded.back() == '\n')
    encoded.pop_back();

  return encoded;
}

	/*
		Check https://github.com/enigma-dev/enigma-dev/pull/2358.
	*/
       string json_encode(variant ds_map) {
               if (!enigma_user::ds_map_exists(ds_map)) {
                       DEBUG_MESSAGE("DS map does not exist", MESSAGE_TYPE::M_ERROR);
                       return string("{}");
               }

               Json::Value root(Json::objectValue);
               const int map_size = enigma_user::ds_map_size(ds_map);
               variant key{enigma_user::ds_map_find_first(ds_map)};

               for (int i = 0; i < map_size; ++i) {
                       const string map_key = enigma_user::toString(key);
                       variant value{enigma_user::ds_map_find_value(ds_map, key)};

                       if (value.type == enigma_user::ty_string) {
                               root[map_key] = Json::Value(enigma_user::toString(value));
                       }
                       else if (value.type == enigma_user::ty_real) {
                               const double number = static_cast<double>(value);

                               if (!std::isfinite(number)) {
                                       DEBUG_MESSAGE("Cannot encode a non-finite JSON number.", MESSAGE_TYPE::M_ERROR);
                                       return string("{}");
                               }

                               root[map_key] = Json::Value(number);
                       }
                       else {
                               DEBUG_MESSAGE("Cannot encode an unsupported variant type as JSON.", MESSAGE_TYPE::M_ERROR);
                               return string("{}");
                       }

                       key = enigma_user::ds_map_find_next(ds_map, key);
               }

               Json::FastWriter writer;
               string result = writer.write(root);

               if (!result.empty() && result.back() == '\n')
                       result.pop_back();

               return result;
       }
}
