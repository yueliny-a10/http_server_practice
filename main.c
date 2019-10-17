#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <linux/types.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>

typedef unsigned long long      u64;
typedef unsigned int            u32;
typedef unsigned short          u16;
typedef unsigned char           u8;	

/*
#define	   u64	unsigned long long
#define    u32	unsigned int
#define    u16	unsigned short
#define    u8	unsigned char 
*/

#define HTML_FILE_PATH "/var/www/html"

char *str_404 = "<!doctype html>\n    \
<html><head>\n 			      \
<title>404 Not Found</title>\n        \
</head><body>\n			      \
<h1>Not Found</h1>\n                  \
<address>William Server</address>\n   \
</body></html>\n";


/* http request method definition */
/* abandoned code 
#define GET		0	
#define HEAD		1
#define POST		2
#define PUT		3
#define DELET		4
#define CONNECT		5
#define OPTIONS		6
#define TRACE		7
#define PATCH		8
*/

/* http status code definition        */
/* RFC 7231 6.  Response Status Codes */
#define HTTP_STATUS_CODE_OK			200
#define HTTP_STATUS_CODE_CREATED		201
#define HTTP_STATUS_CODE_ACCEPTED		202
//#define HTTP_STATUS_CODE_NO_CONTENT		204  misunderstanding, so ignore it
#define HTTP_STATUS_CODE_SEE_OTHER		303
#define HTTP_STATUS_CODE_BAD_REQUEST		400
#define HTTP_STATUS_CODE_FORBIDDEN		403
#define HTTP_STATUS_CODE_NOT_FOUND		404
#define HTTP_STATUS_CODE_REQUEST_URI_TOO_LONG	414
#define HTTP_STATUS_CODE_INTERNAL_SERVER_ERROR	500
#define HTTP_STATUS_CODE_NOT_IMPLEMENTED	501


#define BUFF_SIZE 2048

// RFC 7230 3.1.1:
// It is RECOMMENDED that all HTTP senders and recipients
// support, at a minimum, request-line lengths of 8000 octets.
#define URI_LEN   200    


enum http_hdr_state { start, req_line, msg_hdr, msg_body, check_CR, check_LF };


/* prepare for status code array idx like below
   strncat(resp_hdr, str[status code], strlen[status code])
enum http_status_code {
	
	HTTP_STATUS_CODE_OK = 0,
	HTTP_STATUS_CODE_CREATED,
	HTTP_STATUS_CODE_ACCEPTED,
	HTTP_STATUS_CODE_SEE_OTHER,
	HTTP_STATUS_CODE_BAD_REQUEST,
	HTTP_STATUS_CODE_FORBIDDEN,
	HTTP_STATUS_CODE_NOT_FOUND,
	HTTP_STATUS_CODE_REQUEST_URI_TOO_LONG,
	HTTP_STATUS_CODE_INTERNAL_SERVER_ERROR,
	HTTP_STATUS_CODE_NOT_IMPLEMENTED
};
*/

enum http_request_method {

	HTTP_METHOD_GET = 0,
	HTTP_METHOD_HEAD,
	HTTP_METHOD_POST,
	HTTP_METHOD_PUT,
	HTTP_METHOD_DELETE,
	HTTP_METHOD_CONNECT,
	HTTP_METHOD_OPTIONS,
	HTTP_METHOD_TRACE,
	HTTP_METHOD_PATCH,
	HTTP_METHOD_OTHERS
};


struct http_hdr_info {
	
	/*
	u8  req_method: 4,
	    http_version: 4;
	*/
	int req_method;
	char http_version[4];
	int http_status_code;
	char req_target[128];
	u32 host_ip;
	u16 host_port;
	char *msg_hdr;
};


// study RFC 7230 3.2.4 
void http_hdr_fields_parse() {

}


char *http_req_hdr_parser(char *req_buff, struct http_hdr_info *http_req_hdr) {

	//printf("%s\n", req_buff);
	char *hdr_ptr;
	hdr_ptr = req_buff;

	enum http_hdr_state curr_state = start;
	int i = 0;

	while ( *hdr_ptr != '\0' ) {            
		//printf("%c\n", *hdr_ptr);
		
		switch (curr_state) {

			case start:
				/* only parse GET, DELETE, POST, PUT, CONNECT now*/
				if ( (*hdr_ptr == 'C') && (*(hdr_ptr + 1) == 'N') && (*(hdr_ptr + 2) == 'N') && (*(hdr_ptr + 3) == 'E')
					 	   && (*(hdr_ptr + 4) == 'C') && (*(hdr_ptr + 5) == 'T') ) {
					//printf("is CONNECT\n");
					http_req_hdr->req_method = HTTP_METHOD_CONNECT;
					hdr_ptr += 5;
				}
				else if ( (*hdr_ptr == 'D') && (*(hdr_ptr + 1) == 'E') && (*(hdr_ptr + 2) == 'L') && (*(hdr_ptr + 3) == 'E') 
					           && (*(hdr_ptr + 4) == 'T') && (*(hdr_ptr + 5) == 'E') ) {
					//printf("is DELETE \n");
					http_req_hdr->req_method = HTTP_METHOD_DELETE;
					hdr_ptr += 5;
				}
				else if ( (*hdr_ptr == 'G') && (*(hdr_ptr + 1) == 'E') && (*(hdr_ptr + 2) == 'T') ) {
					//printf("is GET \n");
					http_req_hdr->req_method = HTTP_METHOD_GET;
					hdr_ptr += 2; 
				}
				else if ( (*hdr_ptr == 'H') && (*(hdr_ptr + 1) == 'E') && (*(hdr_ptr + 2) == 'A') && (*(hdr_ptr + 3) == 'D') ) {
					//printf("is HEAD \n");
					http_req_hdr->req_method = HTTP_METHOD_HEAD;
					hdr_ptr += 3;
				}
				else if ( *hdr_ptr == 'P' ) {
					if( (*(hdr_ptr + 1) == 'O') && (*(hdr_ptr + 2) == 'S') && (*(hdr_ptr + 3) == 'T') ) {
						//printf("is POST \n");
						http_req_hdr->req_method = HTTP_METHOD_POST;
						hdr_ptr += 3; 
					}
					else if( (*(hdr_ptr + 1) == 'U') && (*(hdr_ptr + 2) == 'T') ) {
						//printf("is PUT \n");
						http_req_hdr->req_method = HTTP_METHOD_PUT;
						hdr_ptr += 2;
					}
					else if ( (*(hdr_ptr + 1) == 'A') && (*(hdr_ptr + 2) == 'T') && (*(hdr_ptr + 3) == 'C') 
											&& (*(hdr_ptr + 4) == 'H') ) {
						//printf("is PATCH \n");
						http_req_hdr->req_method = HTTP_METHOD_PATCH;
						hdr_ptr += 4;
					}
					else {
						http_req_hdr->req_method = HTTP_METHOD_OTHERS;
						http_req_hdr->http_status_code = HTTP_STATUS_CODE_NOT_IMPLEMENTED;
						printf("not implemented \n");
						return NULL;
					}
				}
				else {  // RFC 7230 3.1.1, response 501
					http_req_hdr->req_method = HTTP_METHOD_OTHERS;
					http_req_hdr->http_status_code = HTTP_STATUS_CODE_NOT_IMPLEMENTED;
					printf("not implemented \n");
					return NULL;
				}
				
				if ( *(hdr_ptr + 1) != ' ' ) {
					http_req_hdr->http_status_code = HTTP_STATUS_CODE_BAD_REQUEST;
					printf("is bad request \n");
					return NULL;
				}
				else {
					hdr_ptr++; // point to SP

					if ( *(hdr_ptr + 1) == '\0' ) { // exception for while loop
						http_req_hdr->http_status_code = HTTP_STATUS_CODE_BAD_REQUEST;
                                        	printf("is bad request \n");
                                        	return NULL;
					}
				}
					
				curr_state = req_line;
				break;

			case req_line:
				// RFC 7230 3.1.1.  Request Line
				// request-line   = method SP request-target SP HTTP-version CRLF
			
				/* get request-target uri */
				if ( *hdr_ptr == ' '  ) { // it should be single space
					http_req_hdr->http_status_code = HTTP_STATUS_CODE_BAD_REQUEST;
					printf("bad request\n");
					return NULL;
				}
				else {
					while ( *(hdr_ptr + i) != ' ' ) {

						if ( i < URI_LEN ) {
							http_req_hdr->req_target[i] = *(hdr_ptr + i);
							i++;
						}
						else {  // RFC 7231 6.5.12, response 414 (Request-URI Too Long)
							http_req_hdr->http_status_code = HTTP_STATUS_CODE_REQUEST_URI_TOO_LONG;
							printf("uri too long\n");
						}
					}
				}
				//printf("request target: %s \n", http_req_hdr->req_target);
				hdr_ptr += i;
				hdr_ptr++;
				
				/* http version handle */
  				if ( (*hdr_ptr == 'H') && (*(hdr_ptr + 1) == 'T') && (*(hdr_ptr + 2) == 'T') && (*(hdr_ptr + 3) == 'P') 
					&& (*(hdr_ptr + 4) == '/') ) {
					hdr_ptr += 5;
					i = 0;
					while ( *(hdr_ptr + i) != '\r' ) {   // 0x0d = '\r'
	
						http_req_hdr->http_version[i] = *(hdr_ptr + i);
						i++;	
					}
					//printf("version: %s \n", http_req_hdr->http_version);
					hdr_ptr += (i - 1); 
				}
				else {
					http_req_hdr->http_status_code = HTTP_STATUS_CODE_BAD_REQUEST;
					printf("bad request \n");
				}

				curr_state = check_CR; 	
				break;

			case msg_hdr:
				// need to check? only ascii 0x41 ~ 0x90 here? check RFC?
				if (*(hdr_ptr + 1) == '\r') // informal code
					curr_state = check_CR;
				//http_hdr_fields_parse();
				break;

			case msg_body:
				if (http_req_hdr->req_method == HTTP_METHOD_PUT) {
					// PUT func
				}
				else if (http_req_hdr->req_method == HTTP_METHOD_POST) {
					// POST func
					return hdr_ptr;
				}
				else {
					// ??
				}
				break;

			case check_CR:
				if ( *hdr_ptr == '\r' )
					curr_state = check_LF;
				else {
					http_req_hdr->http_status_code = HTTP_STATUS_CODE_BAD_REQUEST;
					printf("bad request \n");
					return NULL;
				}
				break;

			case check_LF:
				if ( *hdr_ptr == '\n' ) {
					//printf("CRLF \n");
					if ( (*(hdr_ptr + 1) == '\r') && (*(hdr_ptr + 2) == '\n') ) {
						//printf("double CRLF - END \n");
						//return;
						hdr_ptr += 2;
						curr_state = msg_body;
					}
					else
						curr_state = msg_hdr;
				}
				else {
					http_req_hdr->http_status_code = HTTP_STATUS_CODE_BAD_REQUEST;
					printf("bad request \n");
					return NULL;
				}
				break;

			default:
				break;

		}
                
		hdr_ptr++;
	}
	
	return NULL;
}


void http_resp_hdr_fill(char *resp_hdr, struct http_hdr_info *http_req_hdr, char *resp_body) {

        char temp_str[30] = {'\0'};

	// Date info 
	char *wday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	char *mon[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        time_t timep;
        struct tm *p;
        time(&timep);
        p = gmtime(&timep);

	// at RFC 7231 7.1.1.1.  Date/Time Formats
	// ex. Sun, 06 Nov 1994 08:49:37 GMT    ; IMF-fixdate
	sprintf(temp_str, "%s, %d %s %d %02d:%02d:%02d GMT\r\n", wday[p->tm_wday], p->tm_mday, mon[p->tm_mon], 
							(1900 + p->tm_year), p->tm_hour, p->tm_min, p->tm_sec);

	char *http_str = "HTTP/";
	char *my_http_version = "1.0 ";
	char *date_str = "Date: ";
	char *server_info_str = "Server: William-server\r\n";
	char *len_str = "Accept-Ranges: bytes\r\nContent-Length: ";
	char *content_type_str = "Content-Type: text/html\r\n";

	/* RFC 3.1.2 status line */
	// status-line = HTTP-version SP status-code SP reason-phrase CRLF
	strcpy(resp_hdr, http_str);
	strcat(resp_hdr, my_http_version); 
	
	/*
	   consider change to the code like below 
           if (status code is legal) {
                strncat(resp_hdr, str[status code], strlen[status code]);
           }
	*/
	switch (http_req_hdr->http_status_code) {

		case HTTP_STATUS_CODE_OK:
			strcat(resp_hdr, "200 OK\r\n");
			break;
		case HTTP_STATUS_CODE_CREATED:
			strcat(resp_hdr, "201 Created\r\n");
			break; 
		case HTTP_STATUS_CODE_ACCEPTED:
			strcat(resp_hdr, "202 Accepted\r\n");
			break;
		//case HTTP_STATUS_CODE_NO_CONTENT:
		//	strcat(resp_hdr, "204 No Content\r\n");
		//	break;
		case HTTP_STATUS_CODE_SEE_OTHER:
			strcat(resp_hdr, "303 See Other\r\n");
			break;
		case HTTP_STATUS_CODE_BAD_REQUEST:
			strcat(resp_hdr, "400 Bad Request\r\n");
			break;
		case HTTP_STATUS_CODE_FORBIDDEN:
			strcat(resp_hdr, "403 Forbidden\r\n");
			break;
		case HTTP_STATUS_CODE_NOT_FOUND:
			strcat(resp_hdr, "404 Not Found\r\n");
			break;
		case HTTP_STATUS_CODE_INTERNAL_SERVER_ERROR:
			strcat(resp_hdr, "500 Internal Server Error\r\n");
			break;
		case HTTP_STATUS_CODE_NOT_IMPLEMENTED:
			strcat(resp_hdr, "501 Not Implemented\r\n");
			break;
		// 500 for exception
		default:
			strcat(resp_hdr, "500 Internal Server Error\r\n");
			printf("Http status code setting error \n");
			break;
	}

	
	// general info str fill
	strcat(resp_hdr, date_str);
	strcat(resp_hdr, temp_str); // date
	strcat(resp_hdr, server_info_str);
	strcat(resp_hdr, len_str);
	memset(temp_str, '\0', 30);
	sprintf(temp_str, "%ld", strlen(resp_body)); // strlen
	strcat(resp_hdr, temp_str);
	strcat(resp_hdr, "\r\n");
	strcat(resp_hdr, content_type_str);
	strcat(resp_hdr, "\r\n"); // last CRLF
	
	//printf("<<<\n%s\n>>>\n", resp_hdr);
	return;
}


/* get URI file for GET */
void file_get(struct http_hdr_info *http_req_hdr, char *resp_body) {

	FILE *file_for_GET;
	char file_uri[200] = {'\0'};
	strcpy(file_uri, HTML_FILE_PATH);
	strcat(file_uri, http_req_hdr->req_target);
	//printf("PWD: %s \n", file_uri);

	if ( access( file_uri, F_OK ) != -1 ) {
		// file exists
		if ( (file_for_GET = fopen(file_uri, "r")) ) {
			int i = 0;
			char c;

			do {
				c = fgetc(file_for_GET);
				resp_body[i] = c;
				i++;
			} while ( c != EOF );

			resp_body[i - 1] = '\0';
			fclose(file_for_GET);
			//printf("resp_body:\n%s\n", resp_body);
			http_req_hdr->http_status_code = HTTP_STATUS_CODE_OK;
		}
		else {
			// file open failed, response: 500 (Internal Server Error)
			http_req_hdr->http_status_code = HTTP_STATUS_CODE_INTERNAL_SERVER_ERROR;
		}

		// 204 No Content, i misunderstanding this status
		//if ( strlen(resp_body) == 0 ) {
		//	http_req_hdr->http_status_code = HTTP_STATUS_CODE_NO_CONTENT;
		//}
	}
	else {
		// file doesn't exist, reponse: 404 (not found)
		http_req_hdr->http_status_code = HTTP_STATUS_CODE_NOT_FOUND;
		strcpy(resp_body, str_404);
	}

	return;
}


/* prepare for POST */
void file_post(struct http_hdr_info *http_req_hdr, char *msg_body_ptr) {

	FILE *file_for_POST;
	char file_uri[200] = {'\0'};
	strcpy(file_uri, HTML_FILE_PATH);
	strcat(file_uri, http_req_hdr->req_target);

	if ( access( file_uri, F_OK ) == -1 ) {
		// file doesn't exist, create/write a new file
		if ( (file_for_POST = fopen(file_uri, "w")) ) {
			//printf("file_POST start\n");
			while ( *msg_body_ptr != '\0' ) {
				fputc(*msg_body_ptr, file_for_POST);
				msg_body_ptr++;	
			}
			fclose(file_for_POST);
			//printf("file_post over\n");

			// response 201 (Created)
			http_req_hdr->http_status_code = HTTP_STATUS_CODE_CREATED;
		}
		else {
			// file open failed, response 500 (Internal Server Error) 
			http_req_hdr->http_status_code = HTTP_STATUS_CODE_INTERNAL_SERVER_ERROR;
			//printf("file_post() file open failed\n");
		}
	}
	else {
		// file already exist, response 303 (See Other) 
		http_req_hdr->http_status_code = HTTP_STATUS_CODE_SEE_OTHER;
	}

	return;
}


// RFC 7231 4.3.5
// it doesn't describe the situation when the target shouldn't be remove
// i used the 403 for response
void file_delete(struct http_hdr_info *http_req_hdr, char *resp_body) {
	
	char file_uri[200] = {'\0'};
        strcpy(file_uri, HTML_FILE_PATH);
        strcat(file_uri, http_req_hdr->req_target);

	struct stat statbuf;
	if ( lstat(file_uri, &statbuf) == -1 ) {
		// file doesn't exist, reponse: 404 (not found)
		http_req_hdr->http_status_code = HTTP_STATUS_CODE_NOT_FOUND;
                strcpy(resp_body, str_404);
		perror("stat");
		return;
	}
	if ( S_ISDIR(statbuf.st_mode) ) {
		// is dir, not file, response 403 (Forbidden)
		http_req_hdr->http_status_code = HTTP_STATUS_CODE_FORBIDDEN;
	}
	else {
		// is file
		if ( S_ISREG(statbuf.st_mode) ) {
			// is regular file
			remove(file_uri);
			// file remove success, response 202 (Accepted)
			http_req_hdr->http_status_code = HTTP_STATUS_CODE_ACCEPTED;
		}
		else {
			// not regular file, response 403 (Forbidden)
			http_req_hdr->http_status_code = HTTP_STATUS_CODE_FORBIDDEN;
		}
	}

	return;
}


void http_req_process(char *req_buff, char *resp_buff) {

	char resp_hdr[2048] = {'\0'};
	char resp_body[2048] = {'\0'};

        struct http_hdr_info *http_req_hdr = malloc(sizeof(struct http_hdr_info));

	// malloc failed
	if (http_req_hdr == NULL) {
		printf("malloc failed - *http_req_hdr \n");
		return;
	}
	memset(http_req_hdr, 0, sizeof(struct http_hdr_info));


	char *msg_body_ptr;
        msg_body_ptr = http_req_hdr_parser(req_buff, http_req_hdr);


	switch (http_req_hdr->req_method) {

		case HTTP_METHOD_GET:
			file_get(http_req_hdr, resp_body);
		        http_resp_hdr_fill(resp_hdr, http_req_hdr, resp_body);

		        // combine : resp_buff = resp_hdr + resp_body
       			strcat(resp_buff, resp_hdr);
       			strcat(resp_buff, resp_body);
			break;

		case HTTP_METHOD_HEAD:
			http_req_hdr->http_status_code = HTTP_STATUS_CODE_OK;
			http_resp_hdr_fill(resp_hdr, http_req_hdr, resp_body);
			strcat(resp_buff, resp_hdr);
			break;

		case HTTP_METHOD_POST:
			// RFC 7231 4.3.3
			// relative status code: 200, 201, 206, 303, 304, 416

			// not condsider Content-Type and content-length yet
			if ( msg_body_ptr != NULL ) {
				file_post(http_req_hdr, msg_body_ptr);
				http_resp_hdr_fill(resp_hdr, http_req_hdr, resp_body);
				strcat(resp_buff, resp_hdr);
			}
			else {
				// if (body is null)
			}
			
			break;

		case HTTP_METHOD_DELETE:
			// RFC 7231 4.3.5
			// relative status code: 202, 204
			file_delete(http_req_hdr, resp_body);
			http_resp_hdr_fill(resp_hdr, http_req_hdr, resp_body);
			strcat(resp_buff, resp_hdr);
			strcat(resp_buff, resp_body);
			break;

       		case HTTP_METHOD_PUT:
        	case HTTP_METHOD_CONNECT:
        	case HTTP_METHOD_OPTIONS:
        	case HTTP_METHOD_TRACE:
        	case HTTP_METHOD_PATCH:
			// Response :501 Not Implemented
			http_req_hdr->http_status_code = HTTP_STATUS_CODE_NOT_IMPLEMENTED;
			http_resp_hdr_fill(resp_hdr, http_req_hdr, resp_body);
			strcat(resp_buff, resp_hdr);
			printf("This HTTP_METHOD not implemented \n");
			break;

		// for HTTP_METHOD_OTHERS
		default: 
			http_resp_hdr_fill(resp_hdr, http_req_hdr, resp_body);
			strcat(resp_buff, resp_hdr);
			printf("This HTTP_METHOD not implemented \n");
			break;
	}

	free(http_req_hdr);
        return;
}


// fix later
// this code can not handle large header or payload
// should change to streaming process
void *thd_for_recv_send(void *fd) {

	int conn_fd = (intptr_t)fd;
	int res = 0;
	char req_buff[BUFF_SIZE] = {0};

	printf("conn_fd of thd_test: %d\n", conn_fd);
	//printf("Thread: %lu\n", pthread_self());

	/*  Linux manual:
	 *  If no messages are available at the socket, the receive calls wait
	 *  for a message to arrive, unless the socket is nonblocking (see fcntl(2))
	 */
	res = recv(conn_fd, req_buff , sizeof(req_buff), 0);

	// recv() return 0 only when
	// 	1. request a 0-byte buffer
	// 	2. the other peer has disconnected
	if ( res < 0 ) {
		printf("recv() failed \n");
		printf("errno : %d, %s \n", errno, strerror(errno));
		exit(0);
	}

	char resp_buff[BUFF_SIZE] = {'\0'};
	http_req_process(req_buff, resp_buff);


	send(conn_fd, resp_buff, strlen(resp_buff), 0);

	close(conn_fd);
	//printf("Thread: %lu socket closed \n", pthread_self());
	
	return 0;
}


int main(int argc, char **argv) {

	int listen_fd, conn_fd;
	int cli_addr_len;
	struct sockaddr_in serv_addr;
	struct sockaddr_in cli_addr;

	
	if ( (listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 ) {
		printf("socket() failed \n");
		exit(0);	
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(12345);
	
	int ret;
	if ( (ret = bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) != 0 ) {
		printf("bind() failed: %d \n", ret);
		printf("errno : %d \n", errno);
		exit(0);
	}

	if ( listen(listen_fd, 16) != 0 ) {
		printf("listen() failed \n");
		exit(0);
	}
	else {
		printf("listening ... \n");
	}


	//pthread_t tid_array[10];
	//int thd_idx;	

	while (1) {

		cli_addr_len = sizeof(cli_addr);
		if ( (conn_fd = accept(listen_fd, (struct sockaddr *)(&cli_addr), (socklen_t *)&cli_addr_len)) < 0 ) {
			printf("accept() failed \n");
			exit(0);
		}

		// condsider thread pool later
		printf("conn_fd of accept: %d\n", conn_fd);
		pthread_t t;
		pthread_create(&t, NULL, thd_for_recv_send, (void *)(intptr_t)conn_fd);

	}

	return 0;        
}


