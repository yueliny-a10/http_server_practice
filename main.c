#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <linux/types.h>
#include <netinet/in.h>
#include <sys/types.h>
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

/* http status code definition */
#define HTTP_STATUS_CODE_OK			200
#define HTTP_STATUS_CODE_NO_CONTENT		204
#define HTTP_STATUS_CODE_BAD_REQUEST		400
#define HTTP_STATUS_CODE_NOT_FOUND		404
#define HTTP_STATUS_CODE_INTERNEL_SERVER_ERROR	500
#define HTTP_STATUS_CODE_NOT_IMPLEMENTED	501

#define BUFF_SIZE 1500


enum http_hdr_state { start, req_line, msg_hdr, msg_body, CR, LF };

enum http_request_method {

	HTTP_METHOD_GET = 0,
	HTTP_METHOD_HEAD,
	HTTP_METHOD_POST,
	HTTP_METHOD_PUT,
	HTTP_METHOD_DELETE,
	HTTP_METHOD_CONNECT,
	HTTP_METHOD_OPTIONS,
	HTTP_METHOD_TRACE,
	HTTP_METHOD_PATCH
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


void http_req_hdr_parser(char *req_buff, struct http_hdr_info *http_req_hdr) {

	//printf("%s\n", req_buff);
	char *hdr_ptr;
	hdr_ptr = req_buff;

	enum http_hdr_state curr_state = start;
	int i = 0;

	while ( *hdr_ptr != '\0' ) {            
		//printf("%c\n", *hdr_ptr);
		
		switch (curr_state) {

			case start:
				/* only implement GET, POST, PUT, CONNECT now*/
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
					else {
						http_req_hdr->http_status_code = HTTP_STATUS_CODE_BAD_REQUEST;
						//printf("is bad request \n");
						return;
					}
				}
				else {
					http_req_hdr->http_status_code = HTTP_STATUS_CODE_BAD_REQUEST;
					printf("is bad request \n");
					return;
				}
				
				if ( *(hdr_ptr + 1) != ' ') {
					http_req_hdr->http_status_code = HTTP_STATUS_CODE_BAD_REQUEST;
					printf("is bad request \n");
					return;
				}
				else {
					hdr_ptr++;
				}
					
				curr_state = req_line;
				break;

			case req_line:
				/* get request-target uri */
				while ( *(hdr_ptr + i) != ' ' ) {
					http_req_hdr->req_target[i] = *(hdr_ptr + i);
					i++;
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

				curr_state = CR; 	
				break;

			case msg_hdr:
				
				if (*(hdr_ptr + 1) == '\r') // informal code
					curr_state = CR;
				break;

			case msg_body:
				if (http_req_hdr->req_method == HTTP_METHOD_PUT) {
					// PUT func
				}
				else if (http_req_hdr->req_method == HTTP_METHOD_POST) {
					// POST func
				}
				break;

			case CR:
				if ( *hdr_ptr == '\r' )
					curr_state = LF;
				else {
					http_req_hdr->http_status_code = HTTP_STATUS_CODE_BAD_REQUEST;
					printf("bad request \n");
					return;
				}
				break;

			case LF:
				if ( *hdr_ptr == '\n' ) {
					//printf("CRLF \n");
					if ( (*(hdr_ptr + 1) == '\r') && (*(hdr_ptr + 2) == '\n') ) {
						//printf("double CRLF - END \n");
						return;
					}
					else
						curr_state = msg_hdr;
				}
				else {
					http_req_hdr->http_status_code = HTTP_STATUS_CODE_BAD_REQUEST;
					printf("bad request \n");
					return;
				}
				break;

			default:
				break;

		}
                
		hdr_ptr++;
	}
}


void http_resp_hdr_fill(char *resp_hdr, struct http_hdr_info *http_req_hdr, char *resp_body) {

        char temp_str[30] = {'\0'};

	// date info 
	char *wday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	char *mon[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        time_t timep;
        struct tm *p;
        time(&timep);
        p = gmtime(&timep);

	sprintf(temp_str, "%s, %d %s %d %d:%d:%d GMT\r\n", wday[p->tm_wday], p->tm_mday, mon[p->tm_mon], (1900 + p->tm_year),
							p->tm_hour, p->tm_min, p->tm_sec);

	/* RFC 3.1.2 status line */	
	char *http_str = "HTTP/";
	char *my_http_version = "1.0 ";
	char *date_str = "Date: ";
	char *server_info_str = "Server: William-server\r\n";
	char *len_str = "Accept-Ranges: bytes\r\nContent-Length: ";
	char *content_type_str = "Content-Type: text/html\r\n";

	strcpy(resp_hdr, http_str);
	strcat(resp_hdr, my_http_version); 
	
	//strcat(resp_hdr, http_req_hdr->http_version);
	//if (http_req_hdr == HTTP_STATUS_CODE_OK) {
		strcat(resp_hdr, "200 OK\r\n");
	//}
	
	// general info
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
	//else if () {}
	//else {}
	
	
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
		}
		else {
			// file open failed, 500
			http_req_hdr->http_status_code = HTTP_STATUS_CODE_INTERNEL_SERVER_ERROR;
		}
	}
	else {
		// file doesn't exsits, 404
		http_req_hdr->http_status_code = HTTP_STATUS_CODE_NOT_FOUND;
	}

	return;
}


void http_req_process(char *req_buff, struct http_hdr_info *http_req_hdr, char *resp_buff) {

	char resp_hdr[2048] = {'\0'};
	char resp_body[2048] = {'\0'};

        http_req_hdr_parser(req_buff, http_req_hdr);


	switch (http_req_hdr->req_method) {

		case HTTP_METHOD_GET:
			file_get(http_req_hdr, resp_body);
		        http_resp_hdr_fill(resp_hdr, http_req_hdr, resp_body);

		        // combine : resp_buff = resp_hdr + resp_body
       			strcat(resp_buff, resp_hdr);
       			strcat(resp_buff, resp_body);
			break;

		case HTTP_METHOD_HEAD:
			break;

		case HTTP_METHOD_POST:
       		case HTTP_METHOD_PUT:
        	case HTTP_METHOD_DELETE:
        	case HTTP_METHOD_CONNECT:
        	case HTTP_METHOD_OPTIONS:
        	case HTTP_METHOD_TRACE:
        	case HTTP_METHOD_PATCH:
			// 501 Not Implemented
			http_req_hdr->http_status_code = HTTP_STATUS_CODE_NOT_IMPLEMENTED;
			printf("not implement yet \n");
			break;

		default:
			printf("No this HTTP_METHOD \n");
			break;
	}
/*
	http_resp_hdr_fill(resp_hdr, http_req_hdr, resp_body);

	// combine : resp_buff = resp_hdr + resp_body
	strcat(resp_buff, resp_hdr);
	strcat(resp_buff, resp_body);
*/
        return;
}

// rename func name later
void *thd_test(void *fd) {

	int *arg = (int *)fd;
	int conn_fd = *arg;
	int res = 0;
	char req_buff[BUFF_SIZE] = {0};

	//printf("Thread: %lu\n", pthread_self());

	/*
	 *  If no messages are available at the socket, the receive calls wait
	 *  for a message to arrive, unless the socket is nonblocking (see fcntl(2))
	 */
	res = recv(conn_fd, req_buff , sizeof(req_buff), 0);
	if ( res <= 0 ) {
		printf("recv() failed \n");
		exit(0);
	}

	struct http_hdr_info *http_req_hdr = (struct http_hdr_info *)malloc(sizeof(struct http_hdr_info));
	memset(http_req_hdr->req_target, '\0', sizeof(http_req_hdr->req_target));
	memset(http_req_hdr->http_version, '\0', sizeof(http_req_hdr->http_version));

	char resp_buff[2048] = {'\0'};
	http_req_process(req_buff, http_req_hdr, resp_buff);


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

	
	if ( (listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
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

		pthread_t t;
		pthread_create(&t, NULL, thd_test, (void *)&conn_fd);

		//int res = 0;
		//char req_buff[BUFF_SIZE] = {0};

		/*
		res = recv(conn_fd, req_buff , sizeof(req_buff), 0);
		if ( res <= 0 ) {
			printf("recv() failed \n");
			exit(0);
		}

		struct http_hdr_info *http_req_hdr = (struct http_hdr_info *)malloc(sizeof(struct http_hdr_info));
        	memset(http_req_hdr->req_target, '\0', sizeof(http_req_hdr->req_target));
        	memset(http_req_hdr->http_version, '\0', sizeof(http_req_hdr->http_version));

		char resp_buff[2048] = {'\0'};
		http_req_process(req_buff, http_req_hdr, resp_buff);


		send(conn_fd, resp_buff, strlen(resp_buff), 0);	

		close(conn_fd); 
		*/
	}

	return 0;        
}


