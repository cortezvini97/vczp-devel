#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include "fileinfo.h"
#include "cjson/cJSON.h"
#include "vcpk.h"
#include "string.h"
#include <ctype.h>
#include <linux/limits.h>
#include "vczp.h"

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif


char* current_env = NULL;
const char *CONFIG_FILE = "config.json";



cJSON *load_config() {
    if (file_exists(CONFIG_FILE)) {
        FILE *f = fopen(CONFIG_FILE, "r");
        if (f == NULL) {
            perror("Erro ao abrir o arquivo de configuração");
            return NULL;
        }

        fseek(f, 0, SEEK_END);
        long length = ftell(f);
        fseek(f, 0, SEEK_SET);

        char *data = (char *)malloc(length + 1);
        if (data == NULL) {
            perror("Erro ao alocar memória");
            fclose(f);
            return NULL;
        }

        fread(data, 1, length, f);
        data[length] = '\0';

        fclose(f);

        cJSON *config = cJSON_Parse(data);
        free(data);

        if (config == NULL) {
            printf("Erro ao analisar o JSON\n");
            return NULL;
        }

        return config;
    }

    return cJSON_CreateObject();
}

void executar_comandos(const char *comando, const char *mensagem) {
    printf("%s: %s\n", mensagem, comando);

    int resultado = system(comando);
    if (resultado != 0) {
        fprintf(stderr, "Erro ao executar o comando: %s\n", comando);
        exit(1);
    }
}

void to_lowercase(char *str) {
    while (*str) {
        *str = tolower((unsigned char)*str);
        str++;
    }
}

int check_gcc_installed() {
    // Executa o comando 'gcc --version' e verifica o código de retorno
    int retorno = system("gcc --version > /dev/null 2>&1");
    
    // Verifica se o GCC foi encontrado
    if (retorno == 0) {
        return 1; // GCC está instalado
    } else {
        return 0; // GCC não está instalado
    }
}

void help() {
    printf("Uso: vczp [opção] <arquivos>\n");
    printf("vczp-devel --h: help\n");
    printf("vczp-devel --create <file.vcpk>: create vcpk file\n");
    printf("vczp-devel --create-env <env_name>: create env project\n");
    printf("vczp-devel --select-env <env_name>: select env project\n");
    printf("vczp-devel --exit-env: exit env project\n");
    printf("vczp-devel --make <file.vcpk>: build vcpk\n");
}


void process_prebuildsource(cJSON *sections) {
    cJSON *prebuildsource = cJSON_GetObjectItem(sections, "prebuildsource");
    if (prebuildsource != NULL && cJSON_IsArray(prebuildsource)) {
        cJSON *prebuild = NULL;
        cJSON_ArrayForEach(prebuild, prebuildsource) {
            printf("Configurando..\n");
            executar_comandos(prebuild->valuestring, "configurando compilação..");
        }
    }
}

void process_buildSource(cJSON *sections) {
    cJSON *buildsource = cJSON_GetObjectItem(sections, "buildsource");
    if (buildsource != NULL && cJSON_IsArray(buildsource)) {
        cJSON *build = NULL;
        cJSON_ArrayForEach(build, buildsource) {
            printf("Configurando..\n");
            executar_comandos(build->valuestring, "Compilando Source");
        }
    }
}


cJSON* build_package_info(cJSON* sections) {
    // Cria o objeto cJSON que vai conter as informações do pacote
    cJSON* package_info = cJSON_CreateObject();

    if (package_info == NULL) {
        fprintf(stderr, "Failed to create cJSON object\n");
        return NULL;
    }

    // Adiciona os campos ao objeto package_info
    cJSON_AddStringToObject(package_info, "Name", cJSON_GetObjectItem(sections, "Name")->valuestring);
    cJSON_AddStringToObject(package_info, "Version", cJSON_GetObjectItem(sections, "Version")->valuestring);
    cJSON_AddStringToObject(package_info, "Author", cJSON_GetObjectItem(sections, "Author")->valuestring);
    cJSON_AddStringToObject(package_info, "Description", cJSON_GetObjectItem(sections, "Description")->valuestring);
    cJSON_AddStringToObject(package_info, "Architecture", cJSON_GetObjectItem(sections, "Architecture")->valuestring);
    cJSON_AddStringToObject(package_info, "installDir", cJSON_GetObjectItem(sections, "installDir")->valuestring);

    return package_info;
}

void readVcpk(char* file_name) {

    if (!directory_exists(current_env)) {
        printf("Diretório %s não encontrado.\n", current_env);
        exit(1);
    }

    size_t file_path_length = strlen(current_env) + strlen("/vcpks/") + strlen(file_name) + 1;
    char *file_path = (char *)malloc(file_path_length);
    snprintf(file_path, file_path_length, "%s/vcpks/%s", current_env, file_name);

    if (file_path == NULL) {
        perror("Failed to allocate memory");
        exit(1);
    }

    if(!file_exists(file_path))
    {
        printf("Arquivo %s não encontrado.\n", file_name);
        free(file_path); // Libera file_path antes de sair
        exit(1);
    }


    lexicalAnalysis(file_path);

    cJSON* sections = (cJSON*)parse_vcpk(file_path, current_env);

    free(file_path); // Libera file_path após o uso

    if (sections == NULL){
        printf("Ocorreu um erro !");
        exit(1);
    }

    cJSON *sourceType = cJSON_GetObjectItem(sections, "SourceType");
    if (sourceType != NULL && cJSON_IsString(sourceType)) {
        if (strcmp(sourceType->valuestring, "C/C++") == 0) {
            if (check_gcc_installed() == 0) {
                printf("GCC não está instalado.\n");
                cJSON_Delete(sections); // Libera sections antes de sair
                exit(1);
            }

            process_prebuildsource(sections);
            process_buildSource(sections);
            char* nome = cJSON_GetObjectItem(sections, "Name")->valuestring;
            char *nome_lower = my_strdup(nome);
            if (nome_lower == NULL) {
                fprintf(stderr, "Erro ao alocar memória\n");
                cJSON_Delete(sections); // Libera sections antes de sair
                exit(1);
            }

            to_lowercase(nome_lower);

            size_t base_path_length = strlen(current_env) + strlen("/files") + 1;
            char *base_path = (char *)malloc(base_path_length);

            if (base_path == NULL) {
                perror("Failed to allocate memory");
                free(nome_lower); // Libera nome_lower antes de sair
                cJSON_Delete(sections); // Libera sections antes de sair
                exit(1);
            }
            snprintf(base_path, base_path_length, "%s/files", current_env);

            size_t path_length = strlen(base_path) + strlen("/") + strlen(nome_lower) + 1;
            char *path = (char *)malloc(path_length);

            if (path == NULL) {
                perror("Failed to allocate memory");
                free(base_path); // Libera base_path antes de sair
                free(nome_lower); // Libera nome_lower antes de sair
                cJSON_Delete(sections); // Libera sections antes de sair
                exit(1);
            }

            snprintf(path, path_length, "%s/%s", base_path, nome_lower);

            printf("Path: %s\n", path);

            printf("Preparando arquivos\n");

            cJSON *files_info = cJSON_CreateArray();
            cJSON *file_info = create_files_info(path, base_path);
            cJSON_AddItemToArray(files_info, file_info);
            cJSON *final_files_info = inner_file_infos(files_info, base_path);

            cJSON_Delete(files_info); // Corrige a liberação de files_info

            char path_vczp[PATH_MAX];
            sprintf(path_vczp, "%s/output/%s.vczp", current_env, nome_lower);
            int debug_exec = 0;

            cJSON *package_info =  cJSON_CreateObject();
            cJSON_AddStringToObject(package_info, "Name", nome_lower);
            cJSON_AddStringToObject(package_info, "Version", cJSON_GetObjectItem(sections, "Version")->valuestring);
            cJSON_AddStringToObject(package_info, "Author", cJSON_GetObjectItem(sections, "Author")->valuestring);
            cJSON_AddStringToObject(package_info, "Description", cJSON_GetObjectItem(sections, "Description")->valuestring);
            cJSON_AddStringToObject(package_info, "Architecture", cJSON_GetObjectItem(sections, "Architecture")->valuestring);
            cJSON_AddStringToObject(package_info, "installDir", cJSON_GetObjectItem(sections, "installDir")->valuestring);
            cJSON_AddStringToObject(package_info, "files_path", cJSON_PrintUnformatted(cJSON_GetObjectItem(sections, "files")));

            cJSON *commands_install_Arr = cJSON_GetObjectItem(sections, "install");
            cJSON *commands_debuginstall_Arr = cJSON_GetObjectItem(sections, "debuginstall");

            cJSON *commands = cJSON_CreateArray();

            if (commands_install_Arr && cJSON_IsArray(commands_install_Arr)) {
                int array_size = cJSON_GetArraySize(commands_install_Arr);
                for (int i = 0; i < array_size; i++) {
                    cJSON *item = cJSON_GetArrayItem(commands_install_Arr, i);
                    cJSON *commandObj = cJSON_CreateObject();
                    cJSON_AddStringToObject(commandObj, "command", item->valuestring);
                    cJSON_AddStringToObject(commandObj, "type", "release");
                    cJSON_AddItemToArray(commands, commandObj);
                }
            } else {
                printf("O item 'install' não é um array ou não existe.\n");
            }

            cJSON *installDebugDir = cJSON_GetObjectItem(sections, "installDebugDir");
            if (installDebugDir != NULL && cJSON_IsString(installDebugDir) && strlen(installDebugDir->valuestring) > 0) {
                debug_exec = 1;
                cJSON_AddStringToObject(package_info, "installDebugDir", installDebugDir->valuestring);
                if (commands_debuginstall_Arr && cJSON_IsArray(commands_debuginstall_Arr)) {
                    int array_size = cJSON_GetArraySize(commands_debuginstall_Arr);
                    for (int i = 0; i < array_size; i++) {
                        cJSON *item = cJSON_GetArrayItem(commands_debuginstall_Arr, i);
                        cJSON *commandObj = cJSON_CreateObject();
                        cJSON_AddStringToObject(commandObj, "command", item->valuestring);
                        cJSON_AddStringToObject(commandObj, "type", "debug");
                        cJSON_AddItemToArray(commands, commandObj);
                    }
                } else {
                    printf("O item 'debuginstall' não é um array ou não existe.\n");
                }
            }
            
            

            pack_vczp(path_vczp, final_files_info, commands, package_info, base_path, debug_exec);

            if(debug_exec == 1){
                debug(path_vczp, current_env);
            }

            free(nome_lower);
            free(base_path);
            free(path);
            cJSON_Delete(final_files_info); // Libera final_files_info
            cJSON_Delete(package_info); // Libera package_info
            cJSON_Delete(commands); // Libera commands
        } else {
            printf("Tipo de fonte %s não suportado.\n", sourceType->valuestring);
        }
    } else {
        printf("A chave 'SourceType' não foi encontrada ou não é uma string.\n");
    }
    cJSON_Delete(sections);
}


void create_env(const char *env_name) {
    char path[256];

    printf("Creating environment: %s\n", env_name);
    if (mkdir(env_name, 0777) == -1) {
        perror("Error creating main directory");
        return;
    }

    snprintf(path, sizeof(path), "%s/vcpks", env_name);
    if (mkdir(path, 0777) == -1) {
        perror("Error creating vcpks directory");
        return;
    }

    snprintf(path, sizeof(path), "%s/sources", env_name);
    if (mkdir(path, 0777) == -1) {
        perror("Error creating sources directory");
        return;
    }

    snprintf(path, sizeof(path), "%s/files", env_name);
    if (mkdir(path, 0777) == -1) {
        perror("Error creating files directory");
        return;
    }

    snprintf(path, sizeof(path), "%s/output", env_name);
    if (mkdir(path, 0777) == -1) {
        perror("Error creating output directory");
        return;
    }

    snprintf(path, sizeof(path), "%s/debug", env_name);
    if (mkdir(path, 0777) == -1) {
        perror("Error creating debug directory");
        return;
    }

    printf("Environment '%s' created.\n", env_name);
}

void save_config(const char *config) {
    FILE *f = fopen(CONFIG_FILE, "w");
    if (f == NULL) {
        perror("Error opening file");
        exit(1);
    }
    fprintf(f, "%s", config);
    fclose(f);
}

void select_env(const char *env_name) {
    DIR *dir = opendir(env_name);
    if (dir != NULL) {
        closedir(dir);
        current_env = my_strdup(env_name);  // Copia o nome do ambiente para uma nova string
        char config[256];
        snprintf(config, sizeof(config), "{\"current_env\": \"%s\"}", env_name);
        save_config(config);
        printf("Switched to environment '%s'.\n", env_name);
    } else {
        printf("Environment '%s' does not exist.\n", env_name);
    }
}

void exit_env() {
    if (current_env != NULL) {
        free(current_env);  // Libera a memória alocada para current_env
        current_env = NULL;
        if (file_exists(CONFIG_FILE)) {
            remove(CONFIG_FILE);
            printf("Exited the current environment.\n");
        } else {
            printf("Config file does not exist.\n");
        }
    } else {
        printf("No environment to exit.\n");
    }
}

int main(int argc, char *argv[]) {
    cJSON *config = load_config();
    if (config == NULL) {
        printf("Erro ao carregar a configuração.\n");
        return 1;
    }

    cJSON *current_env_json = cJSON_GetObjectItemCaseSensitive(config, "current_env");
    if (cJSON_IsString(current_env_json) && (current_env_json->valuestring != NULL)) {
        current_env = my_strdup(current_env_json->valuestring);  // Copia o valor da configuração para uma nova string
    }

    if (current_env != NULL) {
        printf("Current environment: %s\n", current_env);
    } else {
        printf("current_env é NULL\n");
    }

    cJSON_Delete(config);

    if (argc > 1) {
        char** args = &argv[1];
        if (strcmp(args[0], "--h") == 0) {
            help();
        } else if (strcmp(args[0], "--create") == 0) {
            if (argc == 3) {
                if (strstr(args[1], ".vcpk") != NULL || strstr(args[1], ".VCPK") != NULL) {
                    create_vcpk(args[1], current_env);
                } else {
                    printf("file name requires .vcpk extension.\n");
                    help();
                }
            } else {
                help();
            }
        } else if (strcmp(args[0], "--make") == 0) {
            if (argc == 3) {
                if (strstr(args[1], ".vcpk") != NULL || strstr(args[1], ".VCPK") != NULL) {
                    readVcpk(args[1]);
                } else {
                    printf("file name requires .vcpk extension.\n");
                    help();
                }
            } else {
                help();
            }
        } else if (strcmp(args[0], "--create-env") == 0) {
            if (argc == 3) {
                create_env(args[1]);
            } else {
                help();
            }
        } else if (strcmp(args[0], "--select-env") == 0) {
            if (argc == 3) {
                select_env(args[1]);
            } else {
                help();
            }
        } else if (strcmp(args[0], "--exit-env") == 0) {
            exit_env();
        } else {
            help();
        }
    } else {
        help();
    }

    return 0;
}