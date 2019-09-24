

String Path::Expand(const String& str)
{
  String result;

  const int32 len = str.Len();
  char* base = new char[len+1];
  UnsafeMemory::Memcpy(base, str, len + 1);

  char* it = base;

  while (*it) {
    if (*it == '$') {
      while (*it == '$') {
        ++it;
      }

      char* var_name = it;
      char one_letter_var_name[2];

      //TODO () {}를 구분해서 처리해주어야함. 섞어쓰는것은 좀...
      char brace_open_char = *it;
      if (*it == '(' || *it == '{') {
        int32 level = 1;
        ++it; ++var_name;

        while (level > 0 && *it) {
          if (*it == '(' || *it == '{') {
            level++;
          }
          else if (*it == ')' || *it == '}') {
            level--;
          }

          if (level > 0) {
            ++it;
          }
        }

        *it++ = '\0';
      }
      else {
        var_name = one_letter_var_name;
        var_name[0] = *it++;
        var_name[1] = '\0';
      }

      char* alt = CStringTraits::Strchr(var_name, ':');
      if (alt) {
        alt = '\0';
      }
      else {
        alt = CStringTraits::Strchr(var_name, '\0');
      }

      String var = GetPathVariable(var_name);
      if (var.IsEmpty()) {
        if (*alt) {
          result += Expand(alt);
        }
      }
      else {
        result += Expand(var);
      }
    }
    else {
      result += *it++;
    }
  }

  delete[] base;

  return result
}
