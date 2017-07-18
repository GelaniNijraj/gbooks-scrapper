#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>
#include "jsmn.h"

struct string {
  char *memory;
  size_t size;
};

struct page{
	char *id;
	int order;
};

void str_print(char *str, int start, int end){
	for(; start < end; start++)
		printf("%c", str[start]);
}

int str_matches(char *str, int start, int end, const char *match){
	int i;

	for(i = 0; start < end; start++, i++){
		if(str[start] != match[i] || match[i] == '\0')
			return 0;
	}

	return 1;
}

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp){
  size_t realsize = size * nmemb;
  struct string *mem = (struct string *)userp;

  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}


CURLcode make_get_request(const char *url, struct string *response){
	CURL *curl;
	CURLcode res;
	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();

	if(curl){
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)response);
		res = curl_easy_perform(curl);
	}

	curl_easy_cleanup(curl);
	curl_global_cleanup();
	return res;
}

char* cleanup_response(char *response){
	char *json = strstr(response, "_OC_Run("),  *json_end = NULL;
	if(json != NULL){
		json += 6; // removing _OC_Run(
		json[0] = '\0';
		json++;
		json[0] = '[';
		json_end = strstr(json, ");");
		json_end[0] = ']';
		json_end[1] = '\0';
	}
	return json;
}

int parse_json(char *json, jsmntok_t **tokens){
	int i = 0, r;
	jsmn_parser parser;
	*tokens = malloc(sizeof(jsmntok_t));
	jsmn_init(&parser);

	do{
		i++;
		*tokens = realloc(*tokens, sizeof(jsmntok_t) * i * 256);
		r = jsmn_parse(&parser, json, strlen(json), *tokens, i * 256);
		switch(r){
			case JSMN_ERROR_INVAL:
				printf("- invalid json data\n");
				break;
			case JSMN_ERROR_NOMEM:
				// printf("- not enough memory allocated. increasing memory to %d\n", sizeof(jsmntok_t) * i * 256);
				break;
			case JSMN_ERROR_PART:
				printf("- json string too short\n");
				break;
			default:
				printf("- json parsed successfully, %d tokens parsed\n", r);
		}
	}while(r == JSMN_ERROR_NOMEM);
	return r;
}

int get_pages(jsmntok_t *tokens, char *json, int r){
	int i, j, k, page_count;
	struct page *pages;
	i = -1;
	while(!str_matches(json, tokens[++i].start, tokens[i].end, "page") && i < r);
	str_print(json, tokens[i - 1].start, tokens[i - 1].end);
	page_count = tokens[i].size;
	pages = malloc(sizeof(sizeof pages) * page_count);
	printf("- %d pages found\n", tokens[i].size);
	for(j = 0; j < page_count; ){
		if(tokens[i++].type == JSMN_OBJECT){
			k = i;
			// PID
			while(!str_matches(json, tokens[k].start, tokens[k].end, "order")) k++;
			k++;
			pages[j].id = malloc(sizeof(char) * (end - start));
			// TODO
			// strncpy(pages[j])
			str_print(json, tokens[k].start, tokens[k].end);
			printf(" => ");
			k = i;
			// order
			while(!str_matches(json, tokens[k].start, tokens[k].end, "pid")) k++;
			k++;
			str_print(json, tokens[k].start, tokens[k].end);
			printf("\n");
			j++;
		}
	}
	return pages;
}

int main(){
	int tokens_count, i, j, k, pages;
	char *json = NULL;
	CURLcode res;
	jsmntok_t *tokens;
	struct string response;
	response.memory = malloc(1);
	response.size = 0;

	res = make_get_request("http://books.google.com/books?id=GeEcyY7dE-wC&hl=en&printsec=frontcover&source=gbs_ge_summary_r&cad=0", &response);
	if(res == CURLE_OK){
		printf("- request made successfully\n");
		json = cleanup_response(response.memory);
		tokens_count = parse_json(json, &tokens);
		get_pages(tokens, json, tokens_count);
	}else{
		printf("%s\n", curl_easy_strerror(res));
	}

	return 0;
}