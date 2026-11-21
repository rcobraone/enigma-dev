/********************************************************************************\
**                                                                              **
**  Copyright (C) 2008 Josh Ventura                                             **
**  Copyright (C) 2014 Seth N. Hetu                                             **
**                                                                              **
**  This file is a part of the ENIGMA Development Environment.                  **
**                                                                              **
**                                                                              **
**  ENIGMA is free software: you can redistribute it and/or modify it under the **
**  terms of the GNU General Public License as published by the Free Software   **
**  Foundation, version 3 of the license or any later version.                  **
**                                                                              **
**  This application and its source code is distributed AS-IS, WITHOUT ANY      **
**  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS   **
**  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more       **
**  details.                                                                    **
**                                                                              **
**  You should have recieved a copy of the GNU General Public License along     **
**  with this code. If not, see <http://www.gnu.org/licenses/>                  **
**                                                                              **
**  ENIGMA is an environment designed to create games and other programs with a **
**  high-level, fully compilable language. Developers of ENIGMA or anything     **
**  associated with ENIGMA are in no way responsible for its users or           **
**  applications created by its users, or damages caused by the environment     **
**  or programs made in the environment.                                        **
**                                                                              **
\********************************************************************************/


#include <string>
#include <iostream>
#include <cstdio>
#include <limits>
#include <stdexcept>
using namespace std;
#include "darray.h"

#include "general/parse_basics_old.h"
#include "general/macro_integration.h"
#include "compiler/output_locals.h"
#include "languages/language_adapter.h"

#include "settings.h"


typedef size_t pt; //Use size_t as our pos type; helps on 64bit systems.
map<string,char> edl_tokens; // Logarithmic lookup, with token.
typedef map<string,char>::iterator tokiter;

int scope_braceid = 0;
extern string tostring(int);

#include <Storage/definition.h>
static jdi::definition_scope *current_scope;

int dropscope()
{
  if (current_scope && current_scope != main_context->get_global() &&
      current_scope->parent)
  current_scope = current_scope->parent;
  return 0;
}
int quickscope()
{
  if (!current_scope)
    return 0;
	
  jdi::definition_scope* ns = new jdi::definition_scope("{}",current_scope,jdi::DEF_NAMESPACE);
  current_scope->members["{}"+tostring(scope_braceid++)] = ns;
  current_scope = ns;
  return 0;
}
int initscope(string name)
{
  scope_braceid = 0;
  main_context->get_global()->members[name] = current_scope = new jdi::definition_scope(name,main_context->get_global(),jdi::DEF_NAMESPACE);
  return 0;
}
int quicktype(unsigned flags, string name)
{
  if (!current_scope)
    return 0;
	
  current_scope->members[name] = new jdi::definition(name,current_scope,flags | jdi::DEF_TYPENAME);
  return 0;
}

#include <API/context.h>
#include <System/macros.h>
#include <System/lex_cpp.h>

///Remove whitespace, unfold macros,
///And lex code into synt.
//Compatibility considerations:
//'123.45' with '000.00', then '.0' with '00'. Do *not* replace '0.' with '00'.
int parser_ready_input(string &code,string &synt,unsigned int &strc, varray<string> &string_in_code)
{
  string codo = synt = code;
  pt pos = 0, bpos = 0;
  char last_token = ' '; //Space is actually invalid. Heh.
  
  unsigned mymacroind = 0;
  macro_stack_t mymacrostack;
  
  for (;;)
  { if (pos >= code.length()) {
      if (mymacroind)
        mymacrostack[--mymacroind].release(code,pos);
      else break;
      continue;
    }
    
    //cout << synt.substr(0,bpos) << endl;
    
    if (is_letter(code[pos]))
    {
      //This is a word of some sort. Could be a keyword, a type, a macro... maybe just a varname
      const pt spos = pos;
      while (++pos < code.length() && is_letterd(code[pos]));
      const string name = code.substr(spos,pos-spos);
      
      if (name == "div")
      {
        codo[bpos] = 'd', synt[bpos++] = '@';
        codo[bpos] = 'i', synt[bpos++] = '@';
        codo[bpos] = 'v', synt[bpos++] = '@';
        last_token = '@';
        continue;
      }
      if (name == "mod")
      {
        codo[bpos] = 'm', synt[bpos++] = '@';
        codo[bpos] = 'o', synt[bpos++] = '@';
        codo[bpos] = 'd', synt[bpos++] = '@';
        last_token = '@';
        continue;
      }

      jdi::macro_iter_c itm = main_context->get_macros().find(name);
      if (itm != main_context->get_macros().end())
      {
        if (!macro_recurses(name,mymacrostack,mymacroind))
        {
          string macrostr;
          if (itm->second->argc != -1) {
            vector<string> mpvec;
            jdip::macro_function* mf = (jdip::macro_function*)itm->second;
            jdip::lexer_cpp::parse_macro_params(mf, main_context->get_macros(), code.c_str(), pos, code.length(), mpvec, jdip::token_t(), jdi::def_error_handler);
            char *mss, *mse; mf->parse(mpvec, mss, mse, jdip::token_t());
            macrostr = string (mss, mse);
          }
          else
            macrostr = ((jdip::macro_scalar*)itm->second)->value;
          mymacrostack[mymacroind++].grab(name,code,pos);
          code = macrostr; pos = 0;
          codo.append(macrostr.length(),' ');
          synt.append(macrostr.length(),' ');
          continue;
        }
      }
      
      char c = 'n', cprime = 0;
      
      jdi::definition* d;
      tokiter itt = edl_tokens.find(name);
      if (itt != edl_tokens.end()) {
        c = itt->second;
      }
      else if ((d = current_language->look_up(name)))
      {
        if (d->flags & jdi::DEF_TYPENAME)
          c = 't';
        else if (current_language->is_variadic_function(d))
          c = 'V', cprime = 'n';
      }
      else if (name == "then")
        continue; //"Then" is a truly useless keyword. I see no need to preserve it.
      
      if (last_token == c || last_token == cprime)
      {
        if (c == '&' or c == '^' or c == '|') {} //Ignore these tokens
        else if (c != 'r' and c != 't' and c != '!')
          codo[bpos] = synt[bpos] = ' ', bpos++;
        else {
          codo[bpos] = ' ';
          synt[bpos++] = c;
        }
      }
      
      //Copy the identifier and its token over
      for (pt i = 0; i < name.length(); i++) {
        codo[bpos]   = name[i];
        synt[bpos++] = c;
      }
      
      //Accurately reflect newly defined types and structures
      //These will be added as types now, but their innards will be ignored until ENIGMA "link"
      if (c == 'n' and last_token == 'C') { //"class <name>"
        quicktype(jdi::DEF_CLASS, name); //Add the string we used to determine if this token is 'n' as a struct
      }
      
      last_token = c;
      continue;
    }
    else if (is_digit(code[pos]))
    {
      if (code[pos] == '0' && pos + 1 < code.length() && code[pos + 1] == 'x')
        { pos++; goto HEXADECIMAL_LABEL; }
      if (bpos and synt[bpos-1] == '.')
        synt[bpos-1] = '0';
      else //We don't want to remove significant zeroes, only octal-inducing ones
        while (pos < code.length() && code[pos] == '0')
          pos++;
      if (pos < code.length() && is_digit(code[pos]))
        do {
          codo[bpos] = code[pos];
          synt[bpos++] = '0';
        } while (++pos < code.length() && is_digit(code[pos]));
      else
       codo[bpos] = synt[bpos] = last_token = '0', bpos++;
      
      continue;
    }
    if (code[pos] == '"')
    {
      string str;
      const pt spos = pos;
      if (setting::use_cpp_escapes)
      {
        while (++pos < code.length() && code[pos] != '"')
          if (code[pos] == '\\' && pos + 1 < code.length())
            ++pos;
        if (pos >= code.length())
          break;
        str = code.substr(spos,++pos-spos);
      }
      else
      {
        while (++pos < code.length() && code[pos] != '"');
        if (pos >= code.length())
          break;
        str = (code.substr(spos,++pos-spos));
      }
      string_in_code[strc++] = str;
      cout << "\n\n\nCut string " << str << "\n\n\n";
      codo[bpos] = synt[bpos] = last_token = '"', bpos++;
      continue;
    }
    if (code[pos] == '\'')
    {
      string str;
      const pt spos = pos;
      if (setting::use_cpp_escapes)
      {
        while (++pos < code.length() && code[pos] != '\'')
          if (code[pos] == '\\' && pos + 1 < code.length())
            ++pos;
        if (pos >= code.length())
          break;
        str = code.substr(spos,++pos-spos);
      }
      else
      {
        while (++pos < code.length() && code[pos] != '\'');
        if (pos >= code.length())
          break;
        str = (code.substr(spos,++pos-spos));
      }
      string_in_code[strc++] = str;
      codo[bpos] = synt[bpos] = last_token = setting::use_cpp_strings? '\'' : '\"';
      bpos++;
      continue;
    }
    if (code[pos] == '/')
    {
      if (pos + 1 < code.length() && code[pos + 1] == '/')
      {
        while (++pos < code.length() && code[pos] != '\n' && code[pos] != '\r');
        pos += 2;
        while (pos < code.length() && code[pos] != '\n' && code[pos] != '\r')
          ++pos;
        continue;
      }
      else if (pos + 1 < code.length() && code[pos + 1] == '*')
      {
        pos += 2;
        while (pos + 1 < code.length() &&
               !(code[pos] == '*' && code[pos + 1] == '/'))
          ++pos;

        if (pos + 1 < code.length())
          pos += 2;+        else
          pos = code.length();

        continue;
      }
      else if (pos + 1 >= code.length() || code[pos + 1] != '=')
      {
		++pos;
        codo[bpos] = synt[bpos] = last_token = '/', bpos++;
        codo += "(double)";
        synt += "(double)";
        for (int i = 0; "(double)"[i]; i++)
          codo[bpos] = "(double)"[i], synt[bpos] = 'c', bpos++;
        continue;
      }
      codo[bpos] = synt[bpos] = last_token = '/', bpos++;
      continue;
    }
    
    if (code[pos] == '$')
    {
      HEXADECIMAL_LABEL:
      codo.append(1,' ');
      synt.append(1,' ');
      codo[bpos] = synt[bpos] = '0', bpos++;
      codo[bpos] = 'x', synt[bpos] = '0', bpos++;
      while (++pos < code.length() && is_hexdigit(code[pos]))
        codo[bpos] = code[pos], synt[bpos] = '0', bpos++;
      continue;
    }
    
    if (code[pos] == '{')
      quickscope();
    else if (code[pos] == '}')
      dropscope();
    
    //Wasn't anything usable
    if (!is_useless(code[pos]))
      codo[bpos] = synt[bpos] = last_token = code[pos], bpos++;
    
    pos++;
  }
  
  code = codo;
  
  code.erase(bpos);
  synt.erase(bpos);
  
  return 0;
}

//Has conditional popping based on an attribute pushed with the value
struct stackif
{
  stackif* prev;
  char value,popifc;
  
  stackif(void):prev(NULL) { }
  stackif(char v):prev(NULL),value(v) { }
  stackif(stackif* p,char v,char i):prev(p),value(v),popifc(i) { }
  
  stackif* push(char v,char i)
  {
    stackif* r = new stackif(this,v,i);
    return r;
  }
  stackif* popif(char i)
  {
    if (prev == NULL or popifc != i)
      return this;

    stackif* r = prev;
    delete this;
    return r;
  }

  operator char()
  {
    return value;
  }
};

//Check if semicolon is needed here
inline bool needs_semi(char c1,char c2)
{
  if (c1 == c2) return 0; //if the two tokens are the same, we assume they are one word; if they are
  return (c1 == 'b' or c1 == 'n' or c1 == '0' or c1 == '"' or c1 == ')' or c1 == ']')
  and (is_letterd(c2) or c2=='"' or c2=='{' or c2=='}');
}
inline bool needs_semi_sepd(char c1,char c2)
{
  if (c1 == c2) return 1; //if the two tokens are the same, we assume they are one word; if they are
  return (c1 == 'b' or c1 == 'n' or c1 == '0' or c1 == '"' or c1 == ')' or c1 == ']')
  and (is_letterd(c2) or c2=='"' or c2=='{' or c2=='}');
}


int parser_reinterpret(string &code,string &synt)
{
  cout << "Second pass...\n";
  for (pt pos = 1; pos < code.length(); pos++)
  {
    if (synt[pos] == '0' and synt[pos-1] == '.')
      synt[pos-1] = '0';
    else if (synt[pos] == 't')
    {
      pt rp = pos;
      while (++rp < synt.length() && synt[rp] == 't'); // find the right end

      if (rp >= synt.length())
        break;

      if (synt[rp] == '(') { // constructor e.g, string("test")
        for (pt i = pos; i < rp; i++)
          synt[i] = 'c';
        pos = rp;
      } else if (synt[pos-1] == '(') { // traditional cast e.g, (string)"test"
        const pt sp = pos-1;
        pos = rp;
        if (pos < synt.length() && synt[pos] == ')')
          for (pt i = sp; i <= pos; i++)
            synt[i] = 'c';
      }
      //else if (synt[pos+1]  == '(') // This case doesn't need handled. ttt() is fine as-is.
    }
    else if(synt[pos] == '<' and synt[pos-1] == 't')
    {
      synt[pos++] = 't';
      for (int pc = 1; pc > 0 && pos < synt.length(); synt[pos++] = 't')
      {
        if (synt[pos] == '>') pc--;
        else if (synt[pos] == '<') pc++;
      if (pos >= synt.length())
        break;
      }        
    }
    else if (pos > 1 && pos + 1 < synt.length() &&
             synt[pos] == ':' && synt[pos-1] == ':')
    {
      char t = synt[pos+1];
      synt[pos-1] = synt[pos] = t;
      pt i = pos-2;
      char s = synt[i];
      
      if (s == 'n') {
        synt[i] = t;
        while (i > 0 and synt[--i] == s)
          synt[i] = t;
      }
    }
    else if (synt[pos] == 'V')
    {
      const pt spos = pos;
      while (pos < synt.length())
      {
        synt[pos] = 'n';
        if (++pos >= synt.length() || synt[pos] != 'V')
          break;
      }
      jdi::definition_function *d =
        (jdi::definition_function*)current_language->look_up(
          code.substr(spos,pos-spos));
      const pt epos = pos;
      if (!d)
      {
        pos = epos;
        continue;
      }     
      int en = current_language->function_variadic_after(d);
      if (en == -1) continue;
      cout << "AND EN = " << en  << endl;
      unsigned lvl = 1;
      for (; en && lvl && pos < synt.length(); ++pos)
      {
        if (synt[pos] == '(') { lvl++; continue; }
        if (synt[pos] == ')') { lvl--; continue; }
        if (lvl == 1 and synt[pos] == ',') en--;
      }
      if (lvl != 0)
        continue;
      if (pos >= synt.length())
        break;

      cout << "CHECK POINT" << endl;
      if (!en) {
        code.insert(pos,"(enigma::varargs(),");
        synt.insert(pos,"(nnnnnnnnnnnnnnn(),");
        pos += 19;
        for (unsigned lvl = 1; lvl && pos < synt.length(); ++pos)
        {
          if (synt[pos] == '(') { lvl++; continue; }
          if (synt[pos] == ')') { lvl--; continue; }
        }

        if (pos >= synt.length())
          break;
        
        pos--;
        code.insert(pos,")");
        synt.insert(pos,")");
      }
      cout << "SUXXESS" << endl;
      pos = epos;
    }
    else if (synt[pos] == '(' and synt[pos-1] == ')')
    {
      //We are catching function-dot calls and replacing  (nnnnn()).   with   nnnnn().
      //Since this only affects semantics in GM-style single-line if statements, 
      //  we only perform this check after ')' (e.g., "if (x)").
      const pt spos = pos;
      pt epos = pos;

      //Advance through the next word.
      while (epos+1<synt.length() && synt[++epos] == 'n');
      if (spos+1 < epos) {     //If we found at least one 'n'
        if (synt[epos]=='(') {
          //Advance through the function call, counting brackets.
          int n_left_paren = 1;
          while (epos+1<synt.length() && n_left_paren>0) {
            char it = synt[++epos];
            if (it == '(') { n_left_paren++; }
            if (it == ')') { n_left_paren--; }
          }
          if (n_left_paren==0) { //We matched it completely.
            //Check the next two characters; should be ")."
            if (epos+2<synt.length() && synt[epos+1]==')' && synt[epos+2]=='.') {
              epos++;
              cout <<"function-dot; removing parens around \"" <<code.substr(spos, epos-spos+1) <<"\"\n"; //DEBUG
  
              //Delete; set up the index for the next iteration.
              code.erase(epos,1);
              synt.erase(epos,1);
              code.erase(spos,1);
              synt.erase(spos,1);
              pos = spos-1;
            }
          }
        }
      }
    }
  }
  //cout << "done. " << synt << endl << endl;
  return 0;
}

//Add semicolons
void parser_add_semicolons(string &code,string &synt)
{
  if (code.size() != synt.size())
    return;
	
  //Allocate enough memory to hold all the semicolons we'd ever need
  const pt input_length = code.length();
  if (input_length > (numeric_limits<pt>::max() - 1) / 2)
    throw length_error("ENIGMA parser input is too large");

  const pt buffer_length = input_length * 2 + 1;
  char *codebuf = new char[buffer_length];
  char *syntbuf = new char[buffer_length];
  pt bufpos = 0;
  
  //Add the semicolons in obvious places
  stackif *sy_semi = new stackif(';');
  for (pt pos=0; pos<code.length(); pos++)
  {
    if (synt[pos]==' ') // Automatic semicolon
    {
      codebuf[bufpos] = *sy_semi;
      syntbuf[bufpos++] = *sy_semi;
      sy_semi=sy_semi->popif('s');
    }
    else
    {
      codebuf[bufpos]=code[pos];
      syntbuf[bufpos++]=synt[pos];
      
      if (synt[pos]=='(') {
        if (pos and (synt[pos-1]=='0' or synt[pos-1] == '\'' or synt[pos-1] == '"')) {
          codebuf[bufpos-1] = *sy_semi;
          syntbuf[bufpos-1] = *sy_semi;
          codebuf[bufpos  ] = '(';
          syntbuf[bufpos++] = '(';
        }
        if (sy_semi->prev == NULL)
          sy_semi=sy_semi->push(',','(');
        continue;
      }
      if (synt[pos]==')') { sy_semi=sy_semi->popif('(');    continue; }
      if (synt[pos]==';')
      {
        /*if (synt[pos+1] == ')') {
          bufpos--;
          continue;
        }*/
        while (*sy_semi != ';')
        {
          codebuf[bufpos-1] = *sy_semi;
          syntbuf[bufpos-1] = *sy_semi;
          codebuf[bufpos  ] = ';';
          syntbuf[bufpos++] = ';';
          sy_semi = sy_semi->popif('s');
        }
        sy_semi = sy_semi->popif('s');
        continue;
      }
      if (synt[pos]==':')
      {
        if (pos + 1 < synt.length() && synt[pos+1] == '=')
          continue;
        // TODO: This segment will potentially hurt goto labels
        if (bufpos > 0 && *sy_semi != ';' && *sy_semi != ':')
        {
          codebuf[bufpos-1] = *sy_semi;
          syntbuf[bufpos-1] = *sy_semi;
          codebuf[bufpos  ] = ';';
          syntbuf[bufpos++] = ';';
          sy_semi = sy_semi->popif('s');
        }
        sy_semi=sy_semi->popif('s');
        continue;
      }
      
      if (synt[pos]=='?')
      {
        sy_semi=sy_semi->push(':','s');
        continue;
      }

      if (pos and needs_semi(synt[pos-1],synt[pos]))
      {
        codebuf[bufpos-1] = *sy_semi;
        syntbuf[bufpos-1] = *sy_semi;
        codebuf[bufpos  ] = code[pos];
        syntbuf[bufpos++] = synt[pos];
        sy_semi=sy_semi->popif('s');
      }
      if(pos + 1 < code.length() &&
         ((pos > 2 && synt[pos] == '+' && synt[pos-1] == '+' &&
           synt[pos-2] == 'n' && needs_semi_sepd('n',synt[pos+1])) ||
          (pos > 2 && synt[pos] == '-' && synt[pos-1] == '-' &&
           synt[pos-2] == 'n' && needs_semi_sepd('n',synt[pos+1]))))
      {
        codebuf[bufpos  ] = *sy_semi;
        syntbuf[bufpos++] = *sy_semi;
        sy_semi=sy_semi->popif('s');
      }
    }
    if (synt[pos]=='s')
    {
      string ts; ts.reserve(10);
      ts += code[pos];
      while (++pos < synt.length() && synt[pos] == 's')
      {
        codebuf[bufpos] = code[pos];
        syntbuf[bufpos++] = 's';
        ts += code[pos];
      }

      if (pos >= synt.length())
        break;

      if (synt[pos] != ' ')
        pos--;
      
      if (ts == "case") {
        codebuf[bufpos] = ' ', syntbuf[bufpos] = 's', bufpos++;
        sy_semi=sy_semi->push(':','s');
      }
      else {
        codebuf[bufpos] = syntbuf[bufpos] = '(', bufpos++;
        sy_semi=sy_semi->push(')','s');
      }
    }
    else if (synt[pos] == 'f')
    {
      if (pos + 3 >= synt.length())
        continue;
		
      codebuf[bufpos] = 'o';
      syntbuf[bufpos] = 'f';
      codebuf[++bufpos] = 'r';
      syntbuf[bufpos++] = 'f';

      pos+=3;
      if (synt[pos] != ' ')

      if (pos + 3 >= synt.length())
        break;

      pos += 3;
      if (pos < synt.length() && synt[pos] != ' ')
        pos--; //If there's a (, you'll be at it next iteration

      codebuf[bufpos] = syntbuf[bufpos] = '(', bufpos++;

      sy_semi=sy_semi->push(')','s');
      sy_semi=sy_semi->push(';','s');
      sy_semi=sy_semi->push(';','s');
    }
  }

  //Dump the semicolon stack at the end.
  while (sy_semi)
  {
    codebuf[bufpos] = syntbuf[bufpos] = *sy_semi;
    ++bufpos;

    stackif *prev = sy_semi->prev;
    delete sy_semi;
    sy_semi = prev;
  }

  code = string(codebuf,bufpos);
  synt = string(syntbuf,bufpos);

  //Free memory here, lest it leak.
  delete[] codebuf;
  delete[] syntbuf;

  //cout << code << endl << synt << endl << endl;
  //cout << "cp1"; fflush(stdout);
  
  //This part's trickier; add semicolons only after do ... until and do ... while
  pt len = synt.length();
  for (pt pos=0; pos<len; pos++)
  {
    if (synt[pos] == 'r') //For every 'do'
    {
      //Make sure we're at do
      if (pos + 2 >= len ||
          (code[pos] != 'd' || code[pos+1] != 'o') ||
          !(code[pos+2] == ' ' || synt[pos+2] != 'r'))
      {
        while (pos < len && synt[pos] == 'r' && code[pos] != ' ')
          pos++;
        continue;
      }

      //Begin do handling

      pos += 2;
      int lpos = pos;
      int semis = 1, dos = 0;

      while (pos < len) //looking for end of do
      {
        if (synt[pos] == ';')
          semis=0;
        else if (synt[pos]=='s') //if we're at a statement
        {
          //if the statement is while or until
          if (pos + 1 < len && synt[pos+1]=='s' &&
              ((code[pos]=='w' && code[pos+1]=='h') || code[pos]=='u'))
          {
            if (dos == 0) //If this until is our closing until
            {
              if (semis) { // If they said 'do while', leaving out the semicolon
                pos++; continue; // We let it go.
              }

              pos+=6; //Skip to the end of this until( or while(
              int ps = 1; //Track number of parentheses to find end
              while (pos<len and ps)
              {
                if (synt[pos]=='(') ps++;
                else if (synt[pos]==')') ps--;
                pos++;
              }
              if (pos >= len)
                break;
              if (synt[pos] != ';')
              {
                code.insert(pos,";");
                synt.insert(pos,";");
                len++;
              }
              break;
            }
            else
            {
              dos--;
              semis = 0;
              pos += 4;
            }
          }
          else
          semis=1;
        }
        else if (synt[pos]=='r')
        {
          if (pos + 2 < len &&
              code[pos] == 'd' && code[pos+1] == 'o' &&
              (code[pos+2] == ' ' || synt[pos+2] != 'r'))
          {
            dos++;
            pos+=2;
            if (pos < len && code[pos] != ' ')
              pos--;
          }
          else
            while (pos < len && synt[pos] == 'r' && code[pos] != ' ')
              pos++;
        }
        else if (synt[pos]=='{')
        {
          pos++;
          for (int bs = 1; pos<len and bs; pos++)
          {
            if (synt[pos] == '{') bs++;
            else if (synt[pos] == '}') bs--;
          }
          pos--;
          semis--;
        }
        pos++;
      }

      //Go fix any nested do's
      pos = lpos;
    }
  }
  
  //cout << "cp2"; fflush(stdout);
  
  //now we'll do ONE MORE pass, to take care of for(())
  len = code.length();
  for (pt pos = 1; pos + 1 < len; pos++)
  if (pos + 1 < len &&
      synt[pos-1] == 'f' && synt[pos] == '(' && synt[pos+1] == '(')
  {
    const pt sp = pos;

    pos+=2;
    int pl = 1;
    while (pos<len and pl)
    {
      if (synt[pos]=='(') pl++;
      else if (synt[pos]==')') pl--;
      pos++;
    }
    if (pl == 0 && pos < len && code[pos]==')')
    {
      code.erase(pos,1);
      synt.erase(pos,1);
      code.erase(sp,1);
      synt.erase(sp,1);
      len-=2;
    }
    pos = sp;
  }
  cout << "done. "; fflush(stdout);
}


/**
  This part outputs somewhat well-formatted code.

  @func likesaspace(char c, char d)
    @return returns whether or not the two characters (as seen
    in syntax string) @param c and @param d should be separated
    by a space in the outputted code.

  @func print_the_fucker(string code,string synt)
  @summary Outputs the code in a well-formatted fashion.

**/

inline bool likesaspace(char c,char d)
{
  if (c==d)
    return 0;
  if (c==' ' or c=='{' or c=='}' or c==':' or c==';')
    return 0;
  if (d==' ' or d=='{' or d=='}' or d==':' or d==';' or d==',')
    return 0;
  if (c=='=' and !is_letterd(d))
    return 0;
  if (d=='=' and !is_letterd(c))
    return 0;
  if ((c=='(' or c=='[') and is_letterd(d))
    return 0;
  if ((d=='(' or d=='[' or d==']' or d==')') and is_letterd(c))
    return 0;
  if (c == '-' and d == '>')
    return 0;
  if (c == '.' or d == '.')
    return 0;
  return 1;
}

#include <fstream>

const char * indent_chars   =  "\n                                \
                                                                  \
                                                                  \
                                                                  \
                                                                  ";
static inline string string_settings_escape(string n)
{
  if (!setting::use_cpp_strings) {
    if (!n.length()) return "\"\"";
    if (n[0] == '\'' and n[n.length()-1] == '\'')
      n[0] = n[n.length() - 1] = '"';
  } else {
    if (!n.length()) {
      return "\'\'";
    }
  }
  for (size_t pos = 1; pos < n.length()-1; pos++)
  {
    switch (n[pos])
    {
      case '\\':
          if (n[pos+1] == '#') // Backslashes can only escape number signs
            n.erase(pos,1); // It happens to be escaping this one
          else
            n.insert(pos++,1,'\\'); // Ordinary backslash; escape it for C++.
        break;
      case '#':
          n[pos] = 'n'; // Newlines are expressed with #
          n.insert(pos++,1,'\\');
        break;
      case '\n': // Newlines are allowed in strings in GML, but not in C
          n[pos] = 'n';
          n.insert(pos++,1,'\\');
        break;
      case '\r': // Handle odd cases
          n[pos] = '\\';
          ++pos;
          if (pos < n.length() && n[pos] == '\n')
            n[pos] = 'n';
          else
            n.insert(pos,1,'n');
        break;
      case '"':
          n.insert(pos++,1,'\\');
        break;
    } 
  }
  return n;
}

void print_to_file(string code,string synt,const unsigned int strc, const varray<string> &string_in_code,int indentmin_b4,ofstream &of)
{
  const int indentmin = indentmin_b4 + 1;
  int indc = 0,tind = 0,pars = 0;
  unsigned str_ind = 0;

  const pt len = code.length();
  for (pt pos = 0; pos < len; pos++)
  {
    switch (synt[pos])
    {
      case '{':
          tind = 0;
          of.write(indent_chars,indentmin+indc);
          if (indc < 256) indc+=2;
          of << '{';
          of.write(indent_chars,indentmin+indc);
        break;
      case '}':
          tind = 0;
          if (indc >= 2)
            indc -= 2;
          else
            indc = 0;
          of.write(indent_chars,indentmin+indc);
          of << '}';
          of.write(indent_chars,indentmin+indc);
        break;
      case ';':
        if (pars)
        {
          of << code[pos];
          of << ' ';
          break;
        }
        if (tind) tind = 0;
        case ':':
          of << code[pos];
          of.write(indent_chars,indentmin+indc+tind);
        break;
      case '(':
          if (pars) pars++;
          of << '(';
        break;
      case ')':
          if (pars) pars--;
          of << ')';
          if (pars == 1)
          {
            pars = 0;
            if (pos + 1 < len && synt[pos+1] != '{' && synt[pos+1] != ';')
            {
              tind+=2;
              of.write(indent_chars,indentmin+indc+tind);
            }
          }
        break;
      case '"': case '\'':
          if (pars) pars--;
            if (str_ind >= strc)
            {
              of << code[pos];
              break;
            }
            of << string_settings_escape(string_in_code[str_ind]).c_str();
            str_ind++;
            if (pos + 2 < len && synt[pos+1] == '+' && synt[pos+2] == '"')
              synt[pos+1] = code[pos+1] = ' ';
        break;
      case 's':
      case 'f':
          pars = 1;
          of << code[pos];
        break;

      default:
          of << code[pos];
          if (pos + 1 < len && likesaspace(synt[pos],synt[pos+1]))
            of << ' ';
        break;
    }
  }
  /*if (system("\"/media/HP_PAVILION/Documents and Settings/HP_Owner/Desktop/parseout.txt\""))
  if (system("gedit \"/media/HP_PAVILION/Documents and Settings/HP_Owner/Desktop/parseout.txt\""))
    printf("zomg fnf\r\n");*/
}

int parser_fix_templates(string &code,pt pos,pt spos,string *synt)
{
  if (synt && code.length() != synt->length())
    return 0;
	
  cout << "qass: " << pos << " <" << (synt && code.length() == synt->length() ? "equivalent" : "UNEQUAL") << "> [" << (pos > code.length()) << "]";
  if (pos == 0 || pos > code.length())
    return 0;
  
  pt epos = pos;
  int a2i = 0;
  
  --epos;
  if (code[epos] == '>')
  {
    --epos;
    int ac = 0; bool ga = false; // argument count, given argument = whether or not anything other than whitespace was specified for the arg type
    for (int tbc = 1; tbc > 0 && epos > 0; epos--) // track triangle bracket count 
    {
      if (code[epos] == '<')
        { tbc--; continue; }
      tbc += (code[epos] == '>');
      ac +=  (code[epos] == ',') and ga and (tbc == 1);
      ga |=  (code[epos] != '<'  and code[epos] != '>' and !is_useless(code[epos]));
      ga &=  (code[epos] != ',');
    }
    if (epos == 0 && code[epos] != '<')
      return 0;

    if (tbc != 0)
      return 0;

    a2i = ac + ga;
  }
  
  if (spos >= code.length() || spos > epos)
    return 0;
  pt sp2 = spos;
  while (sp2 < epos && sp2 < code.length() && code[sp2] != '<')
    if (code[sp2++] == ' ') spos = sp2;
  
  cout << " <" << (synt && code.length() == synt->length() ? "equivalent" : "UNEQUAL") << "> [" << (pos > code.length()) << "]";
  cout << "ass: " << spos << ", " << epos << ": " << code.length() << endl;
  string ptname = code.substr(spos,epos-spos+1); // Isolate the potential template's name
  jdi::definition* a = current_language->look_up(ptname);
  if (!a) return 0;
  
  if (a->flags & jdi::DEF_TEMPLATE)
  {
    jdi::definition_template *tmp = (jdi::definition_template*)a;
    if (tmp->params.empty())
      return 0;

    int tmc = static_cast<int>(tmp->params.size()) - 1;
    for (int i = tmc; i >= 0; i--)
      if (tmp->params[i]->default_assignment) tmc = i;
    a2i = tmc - a2i;
    if (a2i < 0)
      return 0;    
    string iseg;
    for (int i = 0; i < a2i;)
      iseg += (++i < a2i) ? "variant," : "variant";
    if (pos >= 1 && code[pos-1] == '>')
    {
      if (pos >= 2 && (code[pos-2] == ',' || code[pos-2] == '<'))
      {
        if (iseg.length())
        {
          code.insert(pos-1, iseg);
          synt && (synt->insert(pos-1, string(iseg.length(),'t')),   true);
          return iseg.length();
        }
        else if (code[pos-2] == ',')
        {
          if (synt)
            synt->erase(pos-2,1);
          return -1;
        }
      }
      else if (iseg.length())
      {
        code.insert(pos-1, ","+iseg),
        synt && (synt->insert(pos-1, string(iseg.length()+1,'t')), true);
        return iseg.length() + 1;
      }
    }
    else
    {
      code.insert(pos, "<"+iseg+">"),
      synt && (synt->insert(pos,   string(iseg.length()+2,'t')),   true);
      return iseg.length() + 2;
    }
  }
  return 0;
}
