# define _CRT_SECURE_NO_WARNINGS

# include <conio.h>
# include <stdint.h>
# include <stdlib.h>
# include <string>
# include <io.h>
# include <errno.h>
# include <string.h>
# include <sys/types.h>
# include <winsock2.h>
# include "md5.h"
# include <iostream>
# include <fstream>
# include <vector>
# include <time.h>
# include <windows.h>
# include <ws2tcpip.h>

# pragma comment (lib, "ws2_32.lib")


# define PORT "17777" // Порт, к которому подключается клиент

#define MAXDATASIZE 64 // максимальное число байт, принимаемых за один раз
#define HEADERSIZE 4
#define HASHSIZE 16

#define IP_ADDR "127.0.0.1"


/* reverse:  переворачиваем строку s на месте */
void reverse(char s[]) {
    int i, j;
    char c;

    for (i = 0, j = strlen(s) - 1; i < j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

void itoa(int n, char s[]) {
    int i, sign;

    if ((sign = n) < 0)  /* записываем знак */
        n = -n;          /* делаем n положительным числом */
    i = 0;
    do {       /* генерируем цифры в обратном порядке */
        s[i++] = n % 10 + '0';   /* берем следующую цифру */
    } while ((n /= 10) > 0);     /* удаляем */
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
}

// Сообщение + хеш
std::string MakeMessage(const char *msg, int len) { // создание пакета с хешем
    char *_msg = new char[len];
    memcpy(_msg, msg, len);
    uint32_t *arr = md5(_msg, len);
    uint8_t *q;
	char *buf = (char *) malloc((len + HASHSIZE) * sizeof(char));
    memset(buf, 0, (len + HASHSIZE) * sizeof(char));
    memcpy(buf, msg, (len + HASHSIZE) * sizeof(char));
    int k = 1;
    for (int i = 3; i >= 0; i--) {
        q = (uint8_t *) &arr[i];
        for (int j = 3; j >= 0; j--) {
            buf[(len + HASHSIZE) - k] = q[j];
            k++;
        }
    }
	free(arr);
    std::string result;
    for (int i = 0; i < len + HASHSIZE; i++) {
        result.push_back(buf[i]);
    }
    free(buf);
	
    return result;
}

int SEND(std::string msg, int sockfd, int i, int len = 0) { // отправка пакета
    std::string message;
    if (len == 0)
        message = MakeMessage("0064", HEADERSIZE);
    else {
        char HeadBuf[HEADERSIZE + HASHSIZE];
        memset(HeadBuf, 0, HEADERSIZE + HASHSIZE);
        itoa(msg.size() % MAXDATASIZE, HeadBuf);
        message = MakeMessage(HeadBuf, HEADERSIZE);
    }
    char bufbool[1];
    bufbool[0] = 0;
    while (bufbool[0] != '1') {
        if (send(sockfd, message.c_str(), HEADERSIZE + HASHSIZE, 0) == -1) {
            perror("recv 123");
        }
        Sleep(100);
        if (recv(sockfd, bufbool, 1, 0) == -1) {
            perror("recv 456");
        } 
    }
    Sleep(100);
    bufbool[0] = 0;
    if (len == 0)
        message = MakeMessage(msg.substr(i * MAXDATASIZE, (i + 1) * MAXDATASIZE).c_str(), MAXDATASIZE);
    else
        message = MakeMessage(msg.substr(i * MAXDATASIZE, i * MAXDATASIZE + msg.size() % MAXDATASIZE).c_str(),
                              msg.size() % MAXDATASIZE);
    while (bufbool[0] != '1') {
		std::cout << message.c_str();
        if (send(sockfd, message.c_str(), MAXDATASIZE + HASHSIZE, 0) == -1) {
            perror("error recv full packet");
			return (1);
		} 
        Sleep(100);
        if (recv(sockfd, bufbool, 1, 0) == -1) {
            perror("error recv not full packet");
			return (1);
		} 
    }
	return (0);
}

void SendMessage(int sockfd, std::string msg) {
    // Количество пакетов
    int packagesNum = (msg.size()) / MAXDATASIZE; // разбиваем строку на пакеты
    if ((msg.size()) % MAXDATASIZE)
        packagesNum++;
    char HeadBuf[HEADERSIZE + HASHSIZE]; // Буфер для мето инфы
    memset(HeadBuf, 0, HEADERSIZE + HASHSIZE); //заполняем нулями
    itoa(packagesNum, HeadBuf);  // Кладем кол-во пакетов в начало
    std::string _msg; //новая строка
    _msg = MakeMessage(HeadBuf, HEADERSIZE); //
    char bufbool[1];
    bufbool[0] = 0;
	//std::string strin = std::to_string(packagesNum);
    // Отправка кол-ва пакетов
    while (bufbool[0] != '1') {
		//std::cout<<_msg.c_str()<<"\n";
		//std::cout<<strin.c_str()<<"\n";
        send(sockfd, _msg.c_str(), HEADERSIZE + HASHSIZE, 0);
        if (recv(sockfd, bufbool, 1, 0) == -1) {
            perror("error recv number of packets");
			return;
        }
    }
    bufbool[0] = 0;
    Sleep(100);
    for (int i = 0; i < packagesNum; i++) { // отправка самих пакетов
        if (i < packagesNum - 1) {
            SEND(msg, sockfd, i);
        } else {
            if (msg.size() % MAXDATASIZE) {
				if (SEND(msg, sockfd, i, msg.size() % MAXDATASIZE) == 1) {
					std::cout << "error recv  not full packet";
					return;
				}
            } else {
				if (SEND(msg, sockfd, i) == 1) {
					std::cout << "error recv  full packet";
					return;
				}
            }
        }
        Sleep(1000);
    }
}
std::string Read_binary(int sockfd, const char *file_name) {
	FILE *f;
		
		int bufer=0;
		int file_size=0;
		
		f = fopen(file_name, "rb");
		while (bufer != EOF){
			bufer = fgetc(f);
			file_size++;
		}
		fclose(f);
		std::cout << file_size;
	std::ifstream infile;
	infile.open(file_name, std::ios::binary | std::ios::in);
	//fseek(
	char *x = new char[file_size-1];
	

	infile.read(x, file_size-1);
	std::string str;
	for (int i=0; i<file_size-1;i++){
		str.push_back(x[i]);}
	std::cout <<'\n'<< str.length();
	std::cout <<'\n'<< str.length();
	send(sockfd, "1", 1, 0);
	return str;
}
std::string Read_txt(int sockfd, const char *file_name) {
	FILE *f;
	//FILE *f1;
	char bufer = ' ';
	int file_size = 0;
	int raws_counter = 0;
	f = fopen(file_name, "rb");
	while (bufer != EOF) {
		bufer = fgetc(f);
		file_size++;
		if (bufer == '\n')
			raws_counter++;
	}
	fclose(f);
	std::cout << file_size;
	//std::cout << '\n' << raws_counter;
	std::ifstream infile;
	infile.open(file_name, std::ios::in);
	//fseek(
	char *x = new char[file_size - 1];


	infile.read(x, file_size - 1);
	std::string str;
	for (int i = 0; i<file_size - 1 - raws_counter;i++) {
		str.push_back(x[i]);
	}
	std::cout << '\n' << str.length() << '\n';

	send(sockfd, "0", 1, 0);
	return str;
}
// получение структуры sockaddr, IPv4 или IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0)
	{
		std::cout << "WSAStartup faild. Error:" << WSAGetLastError();
		return FALSE;
	}
    int sockfd;
	//SOCKET sockfd;
	char s[INET6_ADDRSTRLEN];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(IP_ADDR, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		_getch();
        return 1;
    }
    // cycle for connect & send
    for (p = servinfo; p != NULL; p = p) {//->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, // Создаем сокет -1 - ошибка
                             p->ai_protocol)) == -1 ) {
            perror("client: socket");
            continue;
		}
				
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) { // Устанавливаем соединение
			printf("error on connect");
            closesocket(sockfd);
			//WSACleanup();
            perror("client: connect");
            continue;
        }
		
		inet_ntop(p->ai_family, get_in_addr((struct sockaddr *) p->ai_addr),
			s, sizeof s);
		printf("client: connecting to %s\n", s);
	
			Sleep(2000);
			std::cout << "\n input file name:" << "\n";
			
			std::string file_path;
			std::cin >> file_path;
			std::string::size_type n;
			n = file_path.find(".");
			if (file_path == "exit") {
				send(sockfd, "2", 1, 0);
				closesocket(sockfd);
				break;
			}
			if (file_path.substr(n+1) == "txt")
				SendMessage(sockfd, Read_txt(sockfd, file_path.c_str()));
			else 
				SendMessage(sockfd, Read_binary(sockfd, file_path.c_str()));
				
		closesocket(sockfd);
		
    }
    if (p == NULL) {
        fprintf(stderr, "client: failed to connectn");
		_getch();
        return 2;
    }

 	WSACleanup();
	_getch();
    return 0;
}

