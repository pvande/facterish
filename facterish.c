#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/utsname.h>

#define print(Buffer, String)  if (Buffer == stdout) { fprintf(Buffer, "%s", String); } else { (Buffer += sprintf(Buffer, "%s", String)); }

/** JSON Utilities **/

// Coerces the char* `value` into a valid JSON string and populates `buf`.
static void stringify(char* buf, char* value) {
  if (value == NULL) {
    sprintf(buf, "null");
  }
  else {
    buf += sprintf(buf, "\"");
    int i;
    for (i = 0; value[i] != 0; i++) {
      switch(value[i])
      {
        case '"':  buf += sprintf(buf, "\\\""); break;
        case '\\': buf += sprintf(buf, "\\\\"); break;
        case '\b': buf += sprintf(buf, "\\b");  break;
        case '\f': buf += sprintf(buf, "\\f");  break;
        case '\n': buf += sprintf(buf, "\\n");  break;
        case '\r': buf += sprintf(buf, "\\r");  break;
        case '\t': buf += sprintf(buf, "\\t");  break;
        default:   buf += sprintf(buf, "%c", value[i]);
      }
    }
    buf += sprintf(buf, "\"");
  }
}

// Coerces the int `value` into a valid JSON number and populates `buf`.
// TODO: Fractional values.
static void numify(char* buf, long int value) {
  sprintf(buf, "%ld", value);
}

// Given a buffer (either a FILE* or a char*), this function will take a list
// of (char*, fact_function*) pairs and turn them into a JSON object.
typedef void (*fact_function)(char*);
static void objectify(void* buf, ...) {
  va_list arguments;
  va_start(arguments, buf);

  int i = 0;
  char *name;
  char value[1024];
  fact_function pointer;

  print(buf, "{");

  while (name = va_arg(arguments, char*)) {
    if (i++) print(buf, ", ");

    stringify(value, name);
    print(buf, value);
    print(buf, ": ");

    pointer = va_arg(arguments, fact_function);
    pointer(value);
    print(buf, value);
  }

  va_end(arguments);
  print(buf, "}");
}

// Given a buffer (either a FILE* or a char*), this function will take a list
// of fact_function pointers and turn them into a JSON array.
static void listify(void* buf, ...) {
  va_list arguments;
  va_start(arguments, buf);

  int i = 0;
  char value[1024];
  fact_function pointer;

  print(buf, "[");
  while (pointer = va_arg(arguments, fact_function)) {
    if (i++) print(buf, ", ");
    pointer(value);
    print(buf, value);
  }

  va_end(arguments);
  print(buf, "]");
}

/** Facts **/

void _facter_hostname(char* buf) {
  struct utsname utsname;
  uname(&utsname);
  stringify(buf, utsname.nodename);
}

void _facter_cpus(char* buf) {
  numify(buf, sysconf( _SC_NPROCESSORS_ONLN ));
}

void _facter_kernel(char* buf) {
  struct utsname utsname;
  uname(&utsname);
  stringify(buf, utsname.sysname);
}

void _facter_kernelrelease(char* buf) {
  struct utsname utsname;
  uname(&utsname);
  stringify(buf, utsname.release);
}

void _facter_hardwaremodel(char* buf) {
  struct utsname utsname;
  uname(&utsname);
  stringify(buf, utsname.machine);
}

void _facter_path(char* buf) {
  stringify(buf, getenv("PATH"));
}

void _facter_user(char* buf) {
  stringify(buf, getenv("USER"));
}

void _facter_null_value(char* buf) {
  stringify(buf, NULL);
}

void _facter_complex_string_value(char* buf) {
  stringify(buf, "\"\\/\b\f\n\r\t");
}

void _facter_array_value(char* buf) {
  listify(buf,
    &_facter_hostname,
    &_facter_cpus,
    &_facter_user,
    NULL
  );
}

void _facter_anobject(char* buf) {
  objectify(buf,
    "kernel",        &_facter_kernel,
    "kernelrelease", &_facter_kernelrelease,
    "null",          &_facter_null_value,
    "array",         &_facter_array_value,
    NULL
  );
}


int main() {
  objectify(stdout,
    "hostname",      &_facter_hostname,
    "cpus",          &_facter_cpus,
    "kernel",        &_facter_kernel,
    "kernelrelease", &_facter_kernelrelease,
    "hardwaremodel", &_facter_hardwaremodel,
    "path",          &_facter_path,
    "user",          &_facter_user,
    "string",        &_facter_complex_string_value,
    "object",        &_facter_anobject,
    NULL
  );
  printf("\n");
  return 0;
}
