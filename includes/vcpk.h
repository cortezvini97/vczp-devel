

#ifndef SPEC_H
#define SPEC_H

#define MAX_LINE_LENGTH 1024
#define MAX_SECTIONS 10

void lexicalAnalysis(char *file_path); 
void* parse_vcpk(const char* file_path, const char* current_env);
void create_vcpk(const char *file_name, const char* folder);
char* my_strdup(const char* str);

#endif // SPEC_H