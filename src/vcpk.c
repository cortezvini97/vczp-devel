#include "vcpk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fileinfo.h"
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <cjson/cJSON.h>




static char* remove_quotes(char* str) {
    if (str == NULL || str[0] == '\0') return str;

    size_t len = strlen(str);
    
    if (str[0] == '"' && str[len - 1] == '"') {
        // Move os caracteres internos para frente e insere o terminador nulo
        memmove(str, str + 1, len - 2);
        str[len - 2] = '\0';
    }

    return str;
}


#define DEV_COMMANDS_COUNT 30
#define BASIC_COMMANDS_COUNT 55

static const char *devCommands[DEV_COMMANDS_COUNT] = {
    "gcc", "g++", "make", "automake", "autoconf", "libtool", "binutils", "debugger",
    "rpm-build", "cmake", "flex", "bison", "gdb", "pkg-config", "strace", "valgrind",
    "elfutils", "patch", "rpmdevtools", "git", "subversion", "cvs", "gettext",
    "nasm", "as", "ld", "ar", "strip", "objdump", "objcopy"
};

static const char *basicCommands[BASIC_COMMANDS_COUNT] = {
    "mkdir", "ls", "pwd", "rmdir", "rm", "cp", "mv", "touch", "find", "locate",
    "cat", "more", "less", "head", "tail", "nano", "vi", "vim", "grep", "chmod",
    "chown", "chgrp", "umask", "ps", "top", "kill", "pkill", "jobs", "bg", "fg",
    "df", "du", "free", "uname", "uptime", "reboot", "shutdown", "ping", "ifconfig",
    "ip", "netstat", "ss", "wget", "curl", "history", "man", "echo", "date",
    "tar", "gzip", "gunzip", "zip", "unzip", "configure", "cd"
};

const char** getDevCommands() {
    return devCommands;
}

const char** getBasicCommands() {
    return basicCommands;
}

typedef struct {
    char type[6]; // "Dev" ou "Basic"
    const char *command;
} CommandObj;

static CommandObj *getAllCommands(int *size) {
    int totalCommands = DEV_COMMANDS_COUNT + BASIC_COMMANDS_COUNT;
    CommandObj *allCommands = malloc(totalCommands * sizeof(CommandObj));
    
    if (allCommands == NULL) {
        fprintf(stderr, "Erro ao alocar memória.\n");
        exit(1);
    }

    int index = 0;
    for (int i = 0; i < BASIC_COMMANDS_COUNT; i++) {
        strcpy(allCommands[index].type, "Basic");
        allCommands[index].command = basicCommands[i];
        index++;
    }

    for (int i = 0; i < DEV_COMMANDS_COUNT; i++) {
        strcpy(allCommands[index].type, "Dev");
        allCommands[index].command = devCommands[i];
        index++;
    }

    *size = totalCommands;
    return allCommands;
}


bool isInSection(FILE *file, int current_line_number, const char *section) {
    char line[256];
    int line_number = 0;

    // Rebobina o arquivo para o início
    rewind(file);

    // Itera sobre o arquivo até a linha atual
    while (fgets(line, sizeof(line), file) && line_number <= current_line_number) {
        line_number++;

        // Se chegamos na linha atual, verificamos
        if (line_number == current_line_number) {
            // Agora iteramos para trás (como se estivéssemos percorrendo linhas anteriores)
            for (int i = line_number - 1; i >= 0; i--) {
                // Volta para o início da linha anterior
                fseek(file, -strlen(line) - 1, SEEK_CUR);
                fgets(line, sizeof(line), file);

                char *trimmed_line = strtok(line, "\n");

                // Verifica se a linha anterior começa com a seção que estamos procurando
                if (strncmp(trimmed_line, section, strlen(section)) == 0) {
                    return true;
                }

                // Se a linha anterior começa com qualquer outra seção, pare a verificação
                if (trimmed_line[0] == '%' && strncmp(trimmed_line, section, strlen(section)) != 0) {
                    return false;
                }

                // Volta mais uma linha no arquivo
                fseek(file, -strlen(line) - 1, SEEK_CUR);
            }
        }
    }

    // Se não encontrar a seção, retorna false
    return false;
}

// Verifica se a linha contém comandos proibidos
const char* containsForbiddenCommands(const char* lineText) {
    const char** forbiddenCommands = getDevCommands();
    for (int i = 0; i < DEV_COMMANDS_COUNT; i++) {
        if (strstr(lineText, forbiddenCommands[i]) != NULL) {
            return forbiddenCommands[i];
        }
    }
    return NULL;
}

// Verifica se a linha começa com um comando básico
int checkIfLineStartsWithBasicCommand(const char* lineText) {
    const char** basicCommands = getBasicCommands();
    for (int i = 0; i < BASIC_COMMANDS_COUNT; i++) {
        if (strncmp(lineText, basicCommands[i], strlen(basicCommands[i])) == 0) {
            // Verifica se o restante da linha contém um comando proibido
            const char* remainingText = lineText + strlen(basicCommands[i]);
            while (isspace(*remainingText)) remainingText++;
            if (containsForbiddenCommands(remainingText) != NULL) {
                return 1; // Linha começa com um comando básico e contém comando proibido
            }
        }
    }
    return 0;
}

int isEnclosedInQuotes(const char *line) {
    size_t len = strlen(line);

    if (len < 2) {
        return 0; // Muito curta para estar entre aspas
    }

    // Verificar aspas duplas
    if (line[0] == '"' && line[len - 1] == '"') {
        return 1;
    }

    // Verificar aspas simples
    if (line[0] == '\'' && line[len - 1] == '\'') {
        return 1;
    }

    return 0;
}


void lexicalAnalysis(char *file_path) {
    char line[256];
    FILE *f = fopen(file_path, "r");
    if (!f) {
        fprintf(stderr, "Erro ao abrir o arquivo.\n");
        exit(1);
    }

    int line_number = 0;
    int total_commands = 0;
    CommandObj *commands = getAllCommands(&total_commands);
    
    int in_prebuildSource = 0;
    int in_buildsource = 0;
    int in_install = 0;
    int in_debuginstall = 0;
    int in_files = 0;
    

    while (fgets(line, sizeof(line), f)) {
        line_number++;
        char* trimmed_line = strtok(line, "\n");

        if(trimmed_line == NULL){
            continue;
        }

        if (trimmed_line[0] == '#' || trimmed_line[0] == '%') {

            if (strcmp(trimmed_line, "%prebuildsource") == 0) {
                in_prebuildSource = 1;
            } else if (trimmed_line != NULL && trimmed_line[0] == '%') {
                in_prebuildSource = 0;  // Resetar se for outra seção
            }

            if (strcmp(trimmed_line, "%buildsource") == 0) {
                in_buildsource = 1;
            } else if (trimmed_line != NULL && trimmed_line[0] == '%') {
                in_buildsource = 0;  // Resetar se for outra seção
            }

           

            if (strcmp(trimmed_line, "%install") == 0) {
                in_install = 1;
            } else if (trimmed_line != NULL && trimmed_line[0] == '%') {
                in_install = 0;  // Resetar se for outra seção
            }

            if (strcmp(trimmed_line, "%debuginstall") == 0) {
                in_debuginstall = 1;
            } else if (trimmed_line != NULL && trimmed_line[0] == '%') {
                in_debuginstall = 0;  // Resetar se for outra seção
            }

            if (strcmp(trimmed_line, "%files") == 0) {
                in_files = 1;
            } else if (trimmed_line != NULL && trimmed_line[0] == '%') {
                in_files = 0;  // Resetar se for outra seção
            }

            continue;
        }

        // Parte anterior: verificação de valores entre aspas
        if (strchr(trimmed_line, ':') != NULL) {
            char *colon_pos = strchr(trimmed_line, ':');
            char value[256]; // Assumindo comprimento máximo de 255 caracteres
            strncpy(value, colon_pos + 1, sizeof(value) - 1);
            value[sizeof(value) - 1] = '\0'; // Garante a terminação nula

            // Remove espaços em branco no início de 'value'
            char *value_start = value;
            while (isspace(*value_start)) value_start++;

            // Verifica se o valor está entre aspas, a menos que esteja nas seções %prebuildsource ou %buildsource
            int is_quoted = (*value_start == '"' && value[strlen(value) - 1] == '"') ||
                            (*value_start == '\'' && value[strlen(value) - 1] == '\'');

            if (!is_quoted && !(in_buildsource || in_prebuildSource || in_install || in_debuginstall || in_files)) {
                printf("Erro de sintaxe na linha %d: O valor após o ':' deve estar entre aspas.\n", line_number);
                free(commands);
                fclose(f);
                exit(1);
            }
            continue;
        }
        
        char *remainingText;

        for (int j = 0; j < total_commands; j++) {
            if (strncmp(trimmed_line, commands[j].command, strlen(commands[j].command)) == 0) {
                // Verificar se o comando é seguido por aspas
                remainingText = trimmed_line + strlen(commands[j].command);
                while (*remainingText == ' ' || *remainingText == '\t') remainingText++;

                if (*remainingText != '"' && *remainingText != '\'') {
                    if (!in_buildsource && strcmp(commands[j].type, "Dev") == 0) {
                        break;
                    }
                    printf("Erro de sintaxe na linha %d: O comando '%s' deve ser seguido por uma string com aspas duplas ou simples. EX: %s \"uma string\"\n",
                           line_number, commands[j].command, commands[j].command);
                    free(commands);
                    fclose(f);
                    exit(1);
                }
                break; // Evita verificações redundantes para outros comandos
            }
        }

        

        if(isEnclosedInQuotes(remainingText)){
            continue;
        }

        if (!in_buildsource) {
            const char* forbiddenCommand = containsForbiddenCommands(trimmed_line);
            if (forbiddenCommand != NULL) {
                printf("Erro de sintaxe na linha %d: '%s' deve ser utilizado na seção %%buildsource.\n",
                       line_number, forbiddenCommand);
                free(commands);
                fclose(f);
                exit(1);
            }
        }

    }

    // Libera a memória alocada e fecha o arquivo
    free(commands);
    fclose(f);
}


static void replace_str(char* str, const char* old, const char* new) {
    char buffer[4096];
    char* p;
    int old_len = strlen(old);

    while ((p = strstr(str, old)) != NULL) {
        int len = p - str;
        strcpy(buffer, str);
        buffer[len] = '\0';
        strcat(buffer, new);
        strcat(buffer, p + old_len);
        strcpy(str, buffer);
    }
}

static char* trim(char* str) {
    char *end;

    // Trim leading space
    while (isspace((unsigned char)*str)) str++;

    if (*str == 0)  // All spaces?
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    // Null terminate
    *(end + 1) = '\0';

    return str;
}

static void str_to_lower(char* str) {
    for (char* p = str; *p; p++) {
        *p = tolower((unsigned char)*p);
    }
}

char* my_strdup(const char* str) {
    size_t len = strlen(str) + 1;
    char* copy = (char*)malloc(len);
    if (copy) {
        memcpy(copy, str, len);
    }
    return copy;
}

void create_vcpk(const char *file_name, const char* folder) {
    if (!directory_exists(folder)) {
        printf("Diretório %s não encontrado.\n", folder);
        exit(1);
    }

    // Conteúdo do arquivo
    const char *content =
        "Name: \"Name\"\n"
        "Version: \"Version\"\n"
        "Author: \"Author\"\n"
        "Description:\"Description\"\n"
        "Architecture: \"Architecture\"\n"
        "Licence: \"Licence\"\n"
        "SourceType: \"SourceType\"\n"
        "installDir: \"installDir\"\n"
        "installDebugDir: \"installDebugDir\"\n\n"
        "%prebuildsource\n\n"
        "%buildsource\n\n"
        "%install\n\n"
        "%debuginstall\n\n"
        "%files\n\n";

    // Cria o caminho completo para o arquivo
    char file_path[4096];
    snprintf(file_path, sizeof(file_path), "%s/vcpks/%s", folder, file_name);

    FILE *file_check = fopen(file_path, "r");
    if (file_check != NULL) {
        printf("O arquivo %s já existe.\n", file_name);
        fclose(file_check);
        return;
    }
    
    // Abre o arquivo para escrita
    FILE *f = fopen(file_path, "w");
    if (f == NULL) {
        perror("Erro ao abrir o arquivo");
        exit(1);
    }

    // Escreve o conteúdo no arquivo
    fprintf(f, "%s", content);

    // Fecha o arquivo
    fclose(f);
}


void *parse_vcpk(const char *file_path, const char *current_env) {

    cJSON *sections = cJSON_CreateObject();
    cJSON_AddStringToObject(sections, "Name", "");
    cJSON_AddStringToObject(sections, "Version", "");
    cJSON_AddStringToObject(sections, "Require", "");
    cJSON_AddStringToObject(sections, "Author", "");
    cJSON_AddStringToObject(sections, "installDir", "");
    cJSON_AddStringToObject(sections, "installDebugDir", "");
    cJSON_AddStringToObject(sections, "Description", "");
    cJSON_AddStringToObject(sections, "Architecture", "");
    cJSON_AddStringToObject(sections, "Licence", "");
    cJSON_AddStringToObject(sections, "SourceType", "");

    cJSON_AddArrayToObject(sections, "prebuildsource");
    cJSON_AddArrayToObject(sections, "buildsource");
    cJSON_AddArrayToObject(sections, "install");
    cJSON_AddArrayToObject(sections, "debuginstall");
    cJSON_AddArrayToObject(sections, "files");

    FILE* f = fopen(file_path, "r");
    if (f == NULL) {
        printf("Erro ao abrir arquivo.");
        return NULL;
    }

    

    char line[256];
    char* current_section = NULL;
    while (fgets(line, sizeof(line), f)) {
        char* trimmed_line = strtok(line, "\n");
        if (trimmed_line == NULL || trimmed_line[0] == '#') {
            continue;
        }
        


        if (trimmed_line[0] == '%') {
            free(current_section); 
            current_section = my_strdup(trimmed_line + 1);
            if (current_section != NULL) {
                str_to_lower(current_section); // Function to convert string to lower case
            }
        }else if (current_section != NULL) {
            char* replaced_line = my_strdup(trimmed_line);
            replace_str(replaced_line, "\"", "");
            replace_str(replaced_line, "'", "");
            replace_str(replaced_line, "$current_env", current_env);
            replace_str(replaced_line, "$files", "files");
            replace_str(replaced_line, "$sources", "sources");
            replace_str(replaced_line, "$debug", "debug");

            replaced_line = remove_quotes(replaced_line);

            char* name = cJSON_GetObjectItem(sections, "Name")->valuestring;
            if (name != NULL) {
                str_to_lower(name);
                replace_str(replaced_line, "$name", name);
            }
            char* version = cJSON_GetObjectItem(sections, "Version")->valuestring;
            if (version != NULL) {
                replace_str(replaced_line, "$version", version);
            }

            cJSON* current_array = cJSON_GetObjectItem(sections, current_section);
            if (current_array != NULL) {
                cJSON_AddItemToArray(current_array, cJSON_CreateString(replaced_line));
            }
        }else {
            char* key = strtok(trimmed_line, ":");
            char* value = strtok(NULL, ":");

            if (key != NULL && value != NULL) {
                key = trim(key);
                value = trim(value);
                value = remove_quotes(value);
                cJSON_ReplaceItemInObject(sections, key, cJSON_CreateString(value));
            }
        }
        
    }

    if (current_section != NULL) {
        free(current_section);
    }

    cJSON* installDebugDir = cJSON_GetObjectItem(sections, "installDebugDir");
    if (installDebugDir != NULL) {
        char* debug_dir_value = installDebugDir->valuestring;
        if (strstr(debug_dir_value, "$current_env") != NULL) {
            replace_str(debug_dir_value, "$current_env", current_env);
        }
        if (strstr(debug_dir_value, "$debug") != NULL) {
            replace_str(debug_dir_value, "$debug", "debug");
        }

        cJSON_ReplaceItemInObject(sections, "installDebugDir", cJSON_CreateString(debug_dir_value));
    }
    
    fclose(f);
    
    return sections;
}