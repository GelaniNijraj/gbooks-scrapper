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
	char *id, *url;
	int order;
};

void str_print(char *str, int start, int end){
	for(; start < end; start++)
		printf("%c", str[start]);
}

char* str_get(char *str, int start, int end){
	int i;
	char *n_str = malloc((sizeof(char) * (end - start)) + (sizeof(char) * 1));
	for(i = 0; start < end; start++, i++)
		n_str[i] = str[start];
	n_str[i] = '\0';
	return n_str;
}

int str_matches(char *str, int start, int end, const char *match){
	int i;
	for(i = 0; start < end; start++, i++){
		if(str[start] != match[i] || match[i] == '\0')
			return 0;
	}
	return 1;
}

/* https://stackoverflow.com/a/779960/4997836 */
char* str_replace(char *orig, char *rep, char *with) {
    char *result, *ins, *tmp;
    int len_rep, len_with, len_front, count;

    if (!orig || !rep)
        return NULL;
    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL;
    if (!with)
        with = "";
    len_with = strlen(with);
    ins = orig;
    for (count = 0; tmp = strstr(ins, rep); ++count)
        ins = tmp + len_rep;
    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);
    if (!result)
        return NULL;
    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep;
    }
    strcpy(tmp, orig);
    return result;
}

int find_token(char *str, jsmntok_t *tokens, const char *token, int token_count){
	int i;
	for(i = 0; i < token_count; i++){
		if(str_matches(str, tokens[i].start, tokens[i].end, token))
			return i;
	}
	return -1;
}

char* get_cover_url(char *id){
	char url[] = "http://books.google.com/books";
	char *complete_url = malloc(sizeof(char) * 255);
	complete_url[0] = '\0';
	strcpy(complete_url, url);
	strcat(complete_url, "?id=");
	strcat(complete_url, id);
	strcat(complete_url, "&hl=en&printsec=frontcover&source=gbs_ge_summary_r&cad=0");
	return complete_url;
}

char* get_page_url(char *book_id, char *page_id){
	char url[] = "http://books.google.com/books?id=";
	char *complete_url = malloc(sizeof(char) * 512);
	complete_url[0] = '\0';
	strcpy(complete_url, url);
	strcat(complete_url, book_id);
	strcat(complete_url, "&pg=");
	strcat(complete_url, page_id);
	return complete_url;
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
 
static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream){
  size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
  return written;
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

int parse_book_json(char *json, jsmntok_t **tokens){
	int i = 0, r;
	jsmn_parser parser;

	jsmn_init(&parser);
	r = jsmn_parse(&parser, json, strlen(json), NULL, 0); // get tokens count
	*tokens = malloc(sizeof(jsmntok_t) * r);
	jsmn_init(&parser);
	r = jsmn_parse(&parser, json, strlen(json), *tokens, r);
	// for(i = 0; i < r; i++){
	// 	printf("%s\n", str_get(json, (*tokens)[i].start, (*tokens)[i].end));
	// }
	printf("- %d tokens scanned\n", r);
	return r;
}

struct page* get_pages(char *book_id, jsmntok_t *tokens, char *json, int r, int *page_count){
	int i, j, k, t;
	char *t_str, *t2_str;
	struct page *pages;
	i = -1;
	while(!str_matches(json, tokens[++i].start, tokens[i].end, "page") && i < r);
	str_print(json, tokens[i - 1].start, tokens[i - 1].end);
	*page_count = tokens[i].size;
	pages = malloc(sizeof(struct page) * (*page_count));
	printf("- %d pages found\n", tokens[i].size);
	for(j = 0; j < *page_count; ){
		if(tokens[i++].type == JSMN_OBJECT){
			k = i;
			// PID
			while(!str_matches(json, tokens[k].start, tokens[k].end, "order")) k++;
			k++;
			t_str = str_get(json, tokens[k].start, tokens[k].end);
			sscanf(t_str, "%d", &t);
			free(t_str);
			pages[j].order = t;
			k = i;
			// order
			while(!str_matches(json, tokens[k].start, tokens[k].end, "pid")) k++;
			k++;
			pages[j].id = str_get(json, tokens[k].start, tokens[k].end);
			pages[j].url = get_page_url(book_id, pages[j].id);
			// pages[j].id = t_str;
			// pages[j].id[0] = '\0';
			// free(t_str);
			// printf("%s\n", t_str);
			j++;
		}
	}
	return pages;
}


struct page* get_book_info(char *id, int *page_count){
	int token_count, i;
	char *book_url, *json;
	struct string response;
	CURLcode res;
	jsmntok_t *tokens;

	book_url = get_cover_url(id);
	response.memory = malloc(1);
	response.size = 0;

	res = make_get_request(book_url, &response);
	if(res == CURLE_OK){
		json = cleanup_response(response.memory);
		token_count = parse_book_json(json, &tokens);
		return get_pages(id, tokens, json, token_count, page_count);;
	}else{
		printf("- %s\n", curl_easy_strerror(res));
	}
}

void get_page(struct page page){
	int i;
	char *image_url;
	struct string response, image;
	CURLcode res;


	response.memory = malloc(1);
	response.size = 0;
	res = make_get_request(page.url, &response);
	if(res == CURLE_OK){
		image_url = strstr(response.memory, "preloadImg.src = ");
		image_url += 18;
		for(i = 0; image_url[i] != '\''; i++);
		image_url[i] = '\0';
		image_url = str_replace(image_url, "\\x26", "&");
		image_url = str_replace(image_url, "\\x3d", "=");
		printf("%s\n", image_url);
		// res = make_get_request("https://books.google.co.in/books/content?id=GeEcyY7dE-wC&pg=PP1&img=1&zoom=3&hl=en&sig=ACfU3U3zi7q3GdYSr0cU6Do6cvQhMJHUwA&w=685", &image);
		// if(res == CURLE_OK){
		// 	printf("gotcha!\n");
		// }else{
		// 	printf("- %s\n", curl_easy_strerror(res));
		// }
	}else{
		printf("- %s\n", curl_easy_strerror(res));
	}
}

int main(){
	int token_count, i, j, k, page_count;
	char *prefix;
	char id[] = "GeEcyY7dE-wC";
	struct page *pages;
	pages = get_book_info(id, &page_count);
	for(i = 0; i < 1; i++){
		get_page(pages[i]);
	}
	return 0;
}