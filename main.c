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

typedef unsigned long long      u64;
typedef unsigned int            u32;
typedef unsigned short          u16;
typedef unsigned char           u8;	

#define HTML_PATH "/var/www/html"


/* http request method definition */
#define GET		0	
#define HEAD		1
#define POST		2
#define PUT		3
#define DELET		4
#define CONNECT		5
#define OPTIONS		6
#define TRACE		7
#define PATCH		8


/* http status code definition */
#define OK		200
#define BAD_REQUEST	400


#define buff_size 1500
#define BAD_REQUEST 400


struct http_hdr_info {
	
	/*
	u8  req_method: 4,
	    http_version: 4;
	*/
	int req_method;
	char http_version[4];
	int http_status_code;
	char req_url[128];
	u32 host_ip;
	u16 host_port;
	char *msg_hdr;
 	

};

enum http_hdr_state { start, req_line, msg_hdr, msg_body, CR, LF }; 

char resp_buff[65535] = {'\0'};

FILE *fin, *fout;

void http_hdr_handler(char *buff) {

	printf("%s\n", buff);
	char *hdr_ptr;
	hdr_ptr = buff;

	enum http_hdr_state curr_state = start;

	struct http_hdr_info *http_hdr = (struct http_hdr_info *)malloc(sizeof(struct http_hdr_info));
	memset(http_hdr->req_url, '\0', sizeof(http_hdr->req_url));
	memset(http_hdr->http_version, '\0', sizeof(http_hdr->http_version));

	char http_version[4] = {'\0'};
	int i = 0;
	char my_url[200] = {'\0'}; 
	char c;

	while ( *hdr_ptr != '\0' ) {            
		printf("%c\n", *hdr_ptr);
		
		switch (curr_state) {

			case start:
				if ( (*hdr_ptr == 'C') && (*(hdr_ptr + 1) == 'N') && (*(hdr_ptr + 2) == 'N') && (*(hdr_ptr + 3) == 'E')
					 	   && (*(hdr_ptr + 4) == 'C') && (*(hdr_ptr + 5) == 'T') ) {
					printf("is CONNECT\n");
					http_hdr->req_method = CONNECT;
					hdr_ptr += 5;
				}
				else if ( (*hdr_ptr == 'G') && (*(hdr_ptr + 1) == 'E') && (*(hdr_ptr + 2) == 'T') ) {
					printf("is GET \n");
					http_hdr->req_method = GET;
					hdr_ptr += 2; 
				}
				else if ( *hdr_ptr == 'P' ) {
					if( (*(hdr_ptr + 1) == 'O') && (*(hdr_ptr + 2) == 'S') && (*(hdr_ptr + 3) == 'T') ) {
						printf("is POST \n");
						http_hdr->req_method = POST;
						hdr_ptr += 3; 
					}
					else if( (*(hdr_ptr + 1) == 'U') && (*(hdr_ptr + 2) == 'T') ) {
						printf("is PUT \n");
						http_hdr->req_method = PUT;
						hdr_ptr += 2;
					}
					else {
						http_hdr->http_status_code = BAD_REQUEST;
						printf("is bad request \n");
						return;
					}
				}
				else {
					http_hdr->http_status_code = BAD_REQUEST;
					printf("is bad request \n");
					return;
				}
				
				if ( *(hdr_ptr + 1) != ' ') {
					http_hdr->http_status_code = BAD_REQUEST;
					printf("bad request \n");
					return;
				}
				else {
					hdr_ptr++;
				}
					
				curr_state = req_line;
				break;

			case req_line:
				/* url handle */
				while ( *(hdr_ptr + i) != ' ' ) {
					//url_test[i] = *(hdr_ptr + i);
					http_hdr->req_url[i] = *(hdr_ptr + i);
					i++;
				}
				printf("url: %s \n", http_hdr->req_url);
				hdr_ptr += i;
				hdr_ptr++;
				
				/* read the file of url */
				strcpy(my_url, HTML_PATH);
				strncat(my_url, http_hdr->req_url, (i + 1));
				printf("PWD: %s \n", my_url);
				fin = fopen(my_url, "r");
				if(fin == NULL) {
					printf("Open file failed \n");
					return;
				}
				i = 0;
				do {
					c = fgetc(fin);
					resp_buff[i] = c;
					i++;	
				} while( c != EOF );
				resp_buff[i - 1] = '\0';
				printf("resp_buff:\n %s \n", resp_buff);
				fclose(fin);
				
				/* http version handle */
  				if ( (*hdr_ptr == 'H') && (*(hdr_ptr + 1) == 'T') && (*(hdr_ptr + 2) == 'T') && (*(hdr_ptr + 3) == 'P') 
					&& (*(hdr_ptr + 4) == '/') ) {
					hdr_ptr += 5;
					i = 0;
					while ( *(hdr_ptr + i) != 0x0d ) {   // 0x0d = '/r'
	
						http_version[i] = *(hdr_ptr + i);
						i++;	
					}
					printf("version: %s \n", http_version);
					hdr_ptr += (i - 1); 
				}
				else {
					http_hdr->http_status_code = BAD_REQUEST;
					printf("bad request \n");
				}

				curr_state = CR; 	
				break;

			case msg_hdr:
				
				if (*(hdr_ptr + 1) == '\r') // informal code
					curr_state = CR;
				break;

			case msg_body:
				if (http_hdr->req_method == PUT) {
					// PUT func
				}
				else if (http_hdr->req_method == POST) {
					// POST func
				}
				break;

			case CR:
				if ( *hdr_ptr == '\r' )
					curr_state = LF;
				else {
					http_hdr->http_status_code = BAD_REQUEST;
					printf("bad request \n");
					return;
				}
				break;

			case LF:
				if ( *hdr_ptr == '\n' ) {
					printf("CRLF \n");
					if ( (*(hdr_ptr + 1) == '\r') && (*(hdr_ptr + 2) == '\n') ) {
						printf("double CRLF - END \n");
						return;
					}
					else
						curr_state = msg_hdr;
				}
				else {
					http_hdr->http_status_code = BAD_REQUEST;
					printf("bad request \n");
					return;
				}
				break;

			default:
				break;

		}
                
		hdr_ptr++;
	}
	printf("\n");
			
}


void http_resp_hdr_fill() {

	char resp_hdr[2048] = {'\0'};
	int buff_end;

	/* start line */
	
	/* for http */
	resp_hdr[0] = 'H'; resp_hdr[1] = 'T'; resp_hdr[2] = 'T'; resp_hdr[3] = 'P'; resp_hdr[4] = '/';
	buff_end = 5;
	/*
	int x = 0;
	while( buff_end != 8 ) { // for example: [5]='1', [6]='.', [7]='1'
		resp_hdr[buff_end] = http_hdr->http_version[x];
		x++;
		i++;
	}*/
	//strncat(resp_hdr, http_hdr->http_version, 3);	
	
	/* for https */
	// ignore

	/* msg header */

	/* CRLF */
	resp_hdr[buff_end] = '\r';
	resp_hdr[buff_end] = '\n';
	buff_end += 2;

	/* msg body */

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

	cli_addr_len = sizeof(cli_addr);
	if ( (conn_fd = accept(listen_fd, (struct sockaddr *)(&cli_addr), &cli_addr_len)) < 0 ) {
		printf("accept() failed \n");
		exit(0);
	}

	int res = 0;
	char buff[buff_size] = {0};
	/*
	 *  If no messages are available at the socket, the receive calls wait
	 *  for a message to arrive, unless the socket is nonblocking (see fcntl(2))
	 */
	res = recv(conn_fd, buff , sizeof(buff), 0);
	http_hdr_handler(buff);

	/* temporary use */
	char *test = "HTTP/1.0 200 OK\r\nServer: william_server\r\nAccept-Ranges: bytes\r\nContent-Length: 265\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n";
	char temp[200] = {'\0'};
	strcpy(temp, test);
	strncat(temp, resp_buff, strlen(resp_buff));
	printf("temp string: %s \n strlen: %d \n", temp, strlen(temp));
	send(conn_fd, temp, strlen(temp), 0);
	
	close(conn_fd); 
	printf("socket closed \n");

	return 0;
        
}


