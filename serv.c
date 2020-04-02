#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <unistd.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>


#define PORT 8090
#define PASS_LENGTH 20
#define TRAIN "./db/train"
#define BOOKING "./db/booking"

struct account{
	int id;
	char name[10];
	char pass[PASS_LENGTH];
};

struct train{
	int tid;
	char train_name[20];
	int train_no;
	int av_seats;
	int last_seatno_used;
	int total;
};

struct bookings{
	int bid;
	int type;
	int acc_no;
	int tr_id;
	char trainname[20];
	int seat_start;
	int seat_end;
	int cancelled;
};

char *ACC[3] = {"./db/accounts/customer", "./db/accounts/agent", "./db/accounts/admin"};

void service_cli(int sock);
int login(int sock);
int signup(int sock);
int menu2(int sock, int id);
int menu1(int sock, int id, int type);
void view_booking(int sock, int id, int type);
void view_booking2(int sock, int id, int type, int fd);

int main(){
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd==-1) {
		printf("socket creation failed\n");
		exit(0);
	}
	int optval = 1;
	int optlen = sizeof(optval);
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &optval, optlen)==-1){
		printf("set socket options failed\n");
		exit(0);
	}
	struct sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	sa.sin_port = htons(PORT);

	if(bind(sockfd, (struct sockaddr *)&sa, sizeof(sa))==-1){
		printf("binding port failed\n");
		exit(0);
	}
	if(listen(sockfd, 100)==-1){
		printf("listen failed\n");
		exit(0);
	}
	while(1){ 
		int connectedfd;
		if((connectedfd = accept(sockfd, (struct sockaddr *)NULL, NULL))==-1){
			printf("connection error\n");
			exit(0);
		}
		pthread_t cli;
		if(fork()==0)
			service_cli(connectedfd);
	}

	close(sockfd);
	return 0;
}

void service_cli(int sock){
	int func_id;
	read(sock, &func_id, sizeof(int));
	printf("Client [%d] connected\n", sock-3);
	while(1){		
		if(func_id==1) {login(sock);read(sock, &func_id, sizeof(int));}
		if(func_id==2) {signup(sock);read(sock, &func_id, sizeof(int));}
		if(func_id==3) break;
	}
	close(sock);
	printf("Client [%d] disconnected\n", sock-3);
}

int login(int sock){
	int type, acc_no, fd, valid=1, invalid=0, login_success=0;
	char password[PASS_LENGTH];
	struct account temp;
	read(sock, &type, sizeof(type));
	read(sock, &acc_no, sizeof(acc_no));
	memset(password, 0, PASS_LENGTH);
	read(sock, &password, sizeof(password));

	if((fd = open(ACC[type-1], O_RDWR))==-1)printf("File Error\n");
	struct flock lock;
	
	lock.l_start = (acc_no-1)*sizeof(struct account);
	lock.l_len = sizeof(struct account);
	lock.l_whence = SEEK_SET;
	lock.l_pid = getpid();

	if(type == 1){
		lock.l_type = F_WRLCK;
		fcntl(fd,F_SETLKW, &lock);
		lseek(fd, (acc_no - 1)*sizeof(struct account), SEEK_CUR);
		read(fd, &temp, sizeof(struct account));
		if(temp.id == acc_no){
			if(!strcmp(temp.pass, password)){
				write(sock, &valid, sizeof(valid));
				while(-1!=menu1(sock, temp.id, type));
				login_success = 1;
			}
		}
		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);
		close(fd);
		if(login_success)
		return 3;
	}
	else if(type == 2){
		lock.l_type = F_RDLCK;
		fcntl(fd,F_SETLKW, &lock);
		lseek(fd, (acc_no - 1)*sizeof(struct account), SEEK_CUR);
		read(fd, &temp, sizeof(struct account));
		if(temp.id == acc_no){
			if(!strcmp(temp.pass, password)){
				write(sock, &valid, sizeof(valid));
				while(-1!=menu1(sock, temp.id, type));
				login_success = 1;
			}
		}
		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);
		close(fd);
		if(login_success)
		return 3;	
	}
	else if(type == 3){
		lock.l_type = F_WRLCK;
		fcntl(fd,F_SETLKW, &lock);
		lseek(fd, (acc_no - 1)*sizeof(struct account), SEEK_CUR);
		read(fd, &temp, sizeof(struct account));
		if(temp.id == acc_no){
			if(!strcmp(temp.pass, password)){
				write(sock, &valid, sizeof(valid));
				while(-1!=menu2(sock, temp.id));
				login_success = 1;
			}
		}
		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);
		close(fd);
		if(login_success)
		return 3;
	}
	write(sock, &invalid, sizeof(invalid));
	return 3;
}

int signup(int sock){
	int type, fd, acc_no=0;
	char password[PASS_LENGTH], name[10];
	memset(password, 0,20);
	struct account temp;
	memset(temp.pass, 0, 20);
	read(sock, &type, sizeof(type));
	read(sock, &name, sizeof(name));
	read(sock, &password, sizeof(password));
	//printf("%s\n", password);

	if((fd = open(ACC[type-1], O_RDWR))==-1)printf("File Error\n");
	struct flock lock;
	lock.l_type = F_WRLCK;
	lock.l_len = sizeof(struct account);
	lock.l_start = 0;
	lock.l_whence = SEEK_END;
	lock.l_pid = getpid();

	fcntl(fd, F_SETLKW, &lock);

	int fp = lseek(fd, 0, SEEK_END);
	
	temp.id = fp / sizeof(struct account) + 1;
	strcpy(temp.name, name);
	strcpy(temp.pass, password);
	write(fd, &temp, sizeof(temp));
	write(sock, &temp.id, sizeof(temp.id));

	lock.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &lock);

	close(fd);
	return 3;
}

int menu1(int sock, int id, int type){
	int op_id;
	read(sock, &op_id, sizeof(op_id));
	if(op_id == 1){
		//book a ticket
		int fd = open(TRAIN, O_RDWR);

		struct flock lock;
		lock.l_type = F_RDLCK;
		lock.l_start = 0;
		lock.l_len = 0;
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();
		
		fcntl(fd, F_SETLKW, &lock);

		struct train temp;
		int fp = lseek(fd, 0, SEEK_END);
		int no_of_trains = fp / sizeof(struct train);
		write(sock, &no_of_trains, sizeof(int));
		lseek(fd, 0, SEEK_SET);
		while(fp != lseek(fd, 0, SEEK_CUR)){
			read(fd, &temp, sizeof(struct train));
			write(sock, &temp.tid, sizeof(int));
			write(sock, &temp.train_no, sizeof(int));	
			write(sock, &temp.av_seats, sizeof(int));	
			write(sock, &temp.train_name, sizeof(temp.train_name));		
		}

		int trainid, seats;
		read(sock, &trainid, sizeof(trainid));
		if(trainid>=no_of_trains || trainid<0){
			lock.l_type = F_UNLCK;
			fcntl(fd, F_SETLK, &lock);
			op_id = -1;
			write(sock, &op_id, sizeof(op_id));
			write(sock, &op_id, sizeof(op_id));
			return 1;
		}
		lseek(fd, 0, SEEK_SET);
		lseek(fd, trainid*sizeof(struct train), SEEK_CUR);
		read(fd, &temp, sizeof(struct train));

		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);

		if(!strcmp(temp.train_name, "deleted")){
			op_id = -1;
			write(sock, &op_id, sizeof(op_id));
			write(sock, &op_id, sizeof(op_id));
			return 1;
		}

		write(sock, &temp.av_seats, sizeof(int));
		read(sock, &seats, sizeof(seats));
		
		lock.l_start = trainid*sizeof(struct train);
		lock.l_len = sizeof(struct train);
		lock.l_type = F_WRLCK;
		fcntl(fd, F_SETLKW, &lock);

		lseek(fd, 0, SEEK_SET);
		lseek(fd, trainid*sizeof(struct train), SEEK_CUR);
		read(fd, &temp, sizeof(struct train));

		int avseats = temp.av_seats;

		if(seats>0 && avseats>=seats){
			temp.av_seats -= seats;
			int fd2 = open(BOOKING, O_RDWR);
			struct flock  lock1 = lock;
			lock1.l_start = 0;
			lock1.l_len = 0;
			fcntl(fd2, F_SETLKW, &lock1);
			struct bookings bk;
			int fp2 = lseek(fd2, 0, SEEK_END);
			if(fp2 > 0)
			{
				lseek(fd2, -1*sizeof(struct bookings), SEEK_CUR);
				read(fd2, &bk, sizeof(struct bookings));
				bk.bid++;
			}
			else 
				bk.bid = 0;
			bk.type = type;
			bk.acc_no = id;
			bk.tr_id = trainid;
			bk.cancelled = 0;
			strcpy(bk.trainname, temp.train_name);
			bk.seat_start = temp.last_seatno_used + 1;
			bk.seat_end = temp.last_seatno_used + seats;
			temp.last_seatno_used = bk.seat_end;
			write(fd2, &bk, sizeof(bk));
			lock.l_type = F_UNLCK;
			fcntl(fd2, F_SETLK, &lock);
		 	close(fd2);
		}
		lseek(fd, -1*sizeof(struct train), SEEK_CUR);
		write(fd, &temp, sizeof(temp));
		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);
		close(fd);
		if(seats<=0 || avseats<seats)
			op_id = -1;
		write(sock, &op_id, sizeof(op_id));
		return 1;
	}
	if(op_id == 2){
		view_booking(sock, id, type);
		write(sock, &op_id, sizeof(op_id));
		return 2;
	}
	if(op_id == 3){
		//update booking
		view_booking(sock, id, type);

		int fd1 = open(TRAIN, O_RDWR);
		int fd2 = open(BOOKING, O_RDWR);
		struct flock lock;
		lock.l_type = F_WRLCK;
		lock.l_start = 0;
		lock.l_len = 0;
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();

		fcntl(fd1, F_SETLKW, &lock);
		fcntl(fd2, F_SETLKW, &lock);

		int val;
		struct train temp1;
		struct bookings temp2;
		read(sock, &val, sizeof(int));	//Read the Booking ID to updated
		// read the booking to be updated
		lseek(fd2, 0, SEEK_SET);
		lseek(fd2, val*sizeof(struct bookings), SEEK_CUR);
		read(fd2, &temp2, sizeof(temp2));
		lseek(fd2, -1*sizeof(struct bookings), SEEK_CUR);
		printf("%d %s %d\n", temp2.tr_id, temp2.trainname, temp2.seat_end);
		// read the train details of the booking
		lseek(fd1, 0, SEEK_SET);
		lseek(fd1, (temp2.tr_id)*sizeof(struct train), SEEK_CUR);
		read(fd1, &temp1, sizeof(temp1));
		lseek(fd1, -1*sizeof(struct train), SEEK_CUR);
		printf("%d %s %d\n", temp1.tid, temp1.train_name, temp1.av_seats);


		read(sock, &val, sizeof(int));	//Increase or Decrease


		if(val==1){//increase
			read(sock, &val, sizeof(int)); //No of Seats
			if(temp1.av_seats>= val){
				temp2.cancelled = 1;
				int booked = temp2.seat_end - temp2.seat_start + 1;
				//temp1.av_seats += booked;
				write(fd2, &temp2, sizeof(temp2));

				//Total seats
				int tot_seats = booked + val;

				printf("Tot_seats : %d\n",tot_seats);

				struct bookings bk;

				int fp2 = lseek(fd2, 0, SEEK_END);
				lseek(fd2, -1*sizeof(struct bookings), SEEK_CUR);
				read(fd2, &bk, sizeof(struct bookings));

				bk.bid++;
				bk.type = temp2.type;
				bk.acc_no = temp2.acc_no;
				bk.tr_id = temp2.tr_id;
				bk.cancelled = 0;
				strcpy(bk.trainname, temp2.trainname);
				bk.seat_start = temp1.last_seatno_used + 1;
				bk.seat_end = temp1.last_seatno_used + tot_seats;

				temp1.av_seats -= val;

				printf("Available seats : %d\n",temp1.av_seats);

				temp1.last_seatno_used = bk.seat_end;

				printf("Last ticket : %d\n",temp1.last_seatno_used);

				write(fd2, &bk, sizeof(bk));
				write(fd1, &temp1, sizeof(temp1));
			}
			else{
				op_id = -2;
				write(sock, &op_id, sizeof(op_id));
			}
		}
		else{//decrease			
			read(sock, &val, sizeof(int)); //No of Seats
			if(temp2.seat_end - val < temp2.seat_start){
				temp2.cancelled = 1;
				temp1.av_seats += val;
			}
			else{
				temp2.seat_end -= val;
				temp1.av_seats += val;
			}
			write(fd2, &temp2, sizeof(temp2));
			write(fd1, &temp1, sizeof(temp1));
		}
		lock.l_type = F_UNLCK;
		fcntl(fd1, F_SETLK, &lock);
		fcntl(fd2, F_SETLK, &lock);
		close(fd1);
		close(fd2);
		if(op_id>0)
		write(sock, &op_id, sizeof(op_id));
		return 3;

	}
	if(op_id == 4) {
		//cancel booking
		int val;
		view_booking(sock,id,type);
		read(sock, &val, sizeof(int));
		struct flock lock;
		lock.l_type = F_WRLCK;
		lock.l_start = val*sizeof(struct bookings);
		lock.l_len = sizeof(struct bookings);
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();
		int train_id, no_of_seats;
		int fd = open(BOOKING, O_RDWR);
		struct bookings bk;

		fcntl(fd, F_SETLKW, &lock);
		lseek(fd, lock.l_start, lock.l_whence);
		read(fd, &bk, sizeof(bk));
		lseek(fd, -1*sizeof(bk), SEEK_CUR);
		bk.cancelled = 1;
		train_id = bk.tr_id;
		no_of_seats = bk.seat_end - bk.seat_start + 1;
		write(fd, &bk, sizeof(bk));
		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);
		close(fd);

		fd = open(TRAIN, O_RDWR);
		lock.l_start = train_id*sizeof(struct train);
		lock.l_len = sizeof(struct train);
		lock.l_type = F_WRLCK;
		fcntl(fd, F_SETLKW, &lock);

		struct train tr;
		lseek(fd, lock.l_start, lock.l_whence);
		read(fd, &tr, sizeof(tr));
		tr.av_seats += no_of_seats;
		lseek(fd,-1*sizeof(tr), SEEK_CUR);
		write(fd, &tr, sizeof(tr));

		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);
		close(fd);

		write(sock, &op_id, sizeof(op_id));
		return 4;
	}
	if(op_id == 5) {
		write(sock, &op_id, sizeof(op_id));
		return -1;
	}
	return 0;
}

int menu2(int sock, int id){
	int op_id;
	read(sock, &op_id, sizeof(op_id));
	if(op_id == 1){
		//add a train
		int tid = 0;
		int tno; 
		char tname[20];
		read(sock, &tname, sizeof(tname));
		read(sock, &tno, sizeof(tno));
		struct train temp, temp2;

		temp.tid = tid;
		temp.train_no = tno;
		strcpy(temp.train_name, tname);
		temp.av_seats = 15;
		temp.total = 15;
		temp.last_seatno_used = 0;

		int fd = open(TRAIN, O_RDWR);
		struct flock lock;
		lock.l_type = F_WRLCK;
		lock.l_start = 0;
		lock.l_len = 0;
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();

		fcntl(fd, F_SETLKW, &lock);

		int fp = lseek(fd, 0, SEEK_END);
		if(fp == 0){
			write(fd, &temp, sizeof(temp));
			lock.l_type = F_UNLCK;
			fcntl(fd, F_SETLK, &lock);
			close(fd);
			write(sock, &op_id, sizeof(op_id));
		}
		else{
			lseek(fd, -1 * sizeof(struct train), SEEK_CUR);
			read(fd, &temp2, sizeof(temp2));
			temp.tid = temp2.tid + 1;
			write(fd, &temp, sizeof(temp));
			write(sock, &op_id, sizeof(op_id));	
			lock.l_type = F_UNLCK;
			fcntl(fd, F_SETLK, &lock);
			close(fd);
		}
		return op_id;
	}
	if(op_id == 2){
		int fd = open(TRAIN, O_RDWR);

		struct flock lock;
		lock.l_type = F_WRLCK;
		lock.l_start = 0;
		lock.l_len = 0;
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();
		
		fcntl(fd, F_SETLKW, &lock);

		int fp = lseek(fd, 0, SEEK_END);
		int no_of_trains = fp / sizeof(struct train);
		write(sock, &no_of_trains, sizeof(int));
		lseek(fd, 0, SEEK_SET);
		while(fp != lseek(fd, 0, SEEK_CUR)){
			struct train temp;
			read(fd, &temp, sizeof(struct train));
			write(sock, &temp.tid, sizeof(int));
			write(sock, &temp.train_name, sizeof(temp.train_name));
			write(sock, &temp.train_no, sizeof(int));			
		}
		read(sock, &no_of_trains, sizeof(int));
		if(no_of_trains == 0) write(sock, &no_of_trains, sizeof(int));
		else{
			struct train temp;
			lseek(fd, 0, SEEK_SET);
			lseek(fd, (no_of_trains-1)*sizeof(struct train), SEEK_CUR);
			read(fd, &temp, sizeof(struct train));			
			if(temp.total == temp.av_seats){
				printf("%s is deleted\n", temp.train_name);
				strcpy(temp.train_name,"deleted");
				temp.av_seats = 0;
				lseek(fd, -1*sizeof(struct train), SEEK_CUR);
				write(fd, &temp, sizeof(struct train));
				write(sock, &no_of_trains, sizeof(int));
			}
			else{
				no_of_trains = -5;
				write(sock, &no_of_trains, sizeof(int));	
			}			
		}

		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);
		close(fd);
	}
	if(op_id == 3){
		int fd = open(TRAIN, O_RDWR);

		struct flock lock;
		lock.l_type = F_WRLCK;
		lock.l_start = 0;
		lock.l_len = 0;
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();
		
		fcntl(fd, F_SETLKW, &lock);

		int fp = lseek(fd, 0, SEEK_END);
		int no_of_trains = fp / sizeof(struct train);
		write(sock, &no_of_trains, sizeof(int));
		lseek(fd, 0, SEEK_SET);
		while(fp != lseek(fd, 0, SEEK_CUR)){
			struct train temp;
			read(fd, &temp, sizeof(struct train));
			write(sock, &temp.tid, sizeof(int));
			write(sock, &temp.train_name, sizeof(temp.train_name));
			write(sock, &temp.train_no, sizeof(int));			
		}
		read(sock, &no_of_trains, sizeof(int));

		struct train temp;
		lseek(fd, 0, SEEK_SET);
		lseek(fd, (no_of_trains-1)*sizeof(struct train), SEEK_CUR);
		read(fd, &temp, sizeof(struct train));			

		read(sock, &no_of_trains,sizeof(int));
		if(no_of_trains == 1){
			write(sock, &temp.train_no, sizeof(temp.train_no));
			read(sock, &temp.train_no, sizeof(temp.train_no));
		}
		else{
			write(sock, &temp.av_seats, sizeof(temp.av_seats));
			int current_av = temp.av_seats;
			read(sock, &temp.av_seats, sizeof(temp.av_seats));
			temp.total = temp.total - current_av + temp.av_seats;
		}

		no_of_trains = 3;
		//printf("%s\t%d\t%d\n", temp.train_name, temp.train_no, temp.av_seats);
		lseek(fd, -1*sizeof(struct train), SEEK_CUR);
		write(fd, &temp, sizeof(struct train));
		write(sock, &no_of_trains, sizeof(int));

		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);
		close(fd);
		return op_id;
	}
	if(op_id == 4){
		int type=3, fd, acc_no=0;
		char password[PASS_LENGTH], name[10];
		struct account temp;
		read(sock, &type, sizeof(type));
		read(sock, &name, sizeof(name));
		read(sock, &password, sizeof(password));

		if((fd = open(ACC[type-1], O_RDWR))==-1)printf("File Error\n");
		struct flock lock;
		lock.l_type = F_WRLCK;
		lock.l_start = 0;
		lock.l_len = sizeof(struct account);
		lock.l_whence = SEEK_END;
		lock.l_pid = getpid();

		fcntl(fd, F_SETLKW, &lock);
		int fp = lseek(fd, 0, SEEK_END);
		temp.id = fp / sizeof(struct account) + 1;
		strcpy(temp.name, name);
		strcpy(temp.pass, password);
		write(fd, &temp, sizeof(temp));
		write(sock, &temp.id, sizeof(temp.id));
		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);

		close(fd);
		op_id=4;
		write(sock, &op_id, sizeof(op_id));
		return op_id;
	}
	if(op_id == 5){
		int type, id;
		struct account var;
		read(sock, &type, sizeof(type));

		int fd = open(ACC[type - 1], O_RDWR);
		struct flock lock;
		lock.l_type = F_WRLCK;
		lock.l_start = 0;
		lock.l_whence = SEEK_SET;
		lock.l_len = 0;
		lock.l_pid = getpid();

		fcntl(fd, F_SETLKW, &lock);

		int fp = lseek(fd, 0 , SEEK_END);
		int users = fp/ sizeof(struct account);
		write(sock, &users, sizeof(int));

		lseek(fd, 0, SEEK_SET);
		while(fp != lseek(fd, 0, SEEK_CUR)){
			read(fd, &var, sizeof(struct account));
			write(sock, &var.id, sizeof(var.id));
			write(sock, &var.name, sizeof(var.name));
		}
		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);

		read(sock, &id, sizeof(id));
		if(id == 0){write(sock, &op_id, sizeof(op_id));}
		else{
			lock.l_start = (id-1)*sizeof(struct account);
			lock.l_len = sizeof(struct account);
			lock.l_type = F_WRLCK;
			fcntl(fd, F_SETLKW, &lock);
			lseek(fd, 0, SEEK_SET);
			lseek(fd, (id-1)*sizeof(struct account), SEEK_CUR);
			read(fd, &var, sizeof(struct account));
			lseek(fd, -1*sizeof(struct account), SEEK_CUR);
			strcpy(var.name,"deleted");
			strcpy(var.pass, "");
			write(fd, &var, sizeof(struct account));
			write(sock, &op_id, sizeof(op_id));
		}

		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);

		close(fd);

		return op_id;
	}
	if(op_id == 6){
		int type, id;
		struct account var;
		char updated_name[10], updated_pass[PASS_LENGTH];
		read(sock, &type, sizeof(type));

		int fd = open(ACC[type - 1], O_RDWR);
		struct flock lock;
		lock.l_type = F_RDLCK;
		lock.l_start = 0;
		lock.l_whence = SEEK_SET;
		lock.l_len = 0;
		lock.l_pid = getpid();

		fcntl(fd, F_SETLKW, &lock);

		int fp = lseek(fd, 0 , SEEK_END);
		int users = fp / sizeof(struct account);
		write(sock, &users, sizeof(int));

		lseek(fd, 0, SEEK_SET);
		while(fp > lseek(fd, 0, SEEK_CUR)){
			read(fd, &var, sizeof(struct account));
			write(sock, &var.id, sizeof(var.id));
			write(sock, &var.name, sizeof(var.name));
		}
		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);
		
		lock.l_type = F_WRLCK;
		read(sock, &id, sizeof(id));
		
		lock.l_start = (id-1)*sizeof(struct account);
		lock.l_len = sizeof(struct account);
		fcntl(fd, F_SETLKW, &lock);
		lseek(fd, lock.l_start, SEEK_SET);
		read(fd, &var, sizeof(var));
		write(sock, &var.name, sizeof(var.name));
		write(sock, &var.pass, sizeof(var.pass));

		//printf("%s %s\n", var.name, var.pass);
		
		lseek(fd, lock.l_start, SEEK_SET);
		read(sock, &id, sizeof(id));
		

		if(id == 1)	{
			read(sock, &updated_name, sizeof(updated_name));
			strcpy(var.name, updated_name);
			}
		else if(id == 2) {
			read(sock, &updated_pass, sizeof(updated_pass));
			strcpy(var.pass, updated_pass);
			}

		write(fd, &var, sizeof(var));
		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);
		close(fd);
		write(sock, &op_id, sizeof(op_id));
		return op_id;
	}
	if(op_id == 7){
		//view users
		int fd1 = open(ACC[0], O_RDONLY);
		int fd2 = open(ACC[1], O_RDONLY);
		int fd3 = open(ACC[2], O_RDONLY);

		struct account arr1[100], arr2[100], arr3[100];
		int k1=0, k2=0, k3=0, fp;
		int n = sizeof(struct account);
		int str = sizeof(char)*20;
		
		fp = lseek(fd1, 0, SEEK_END);
		lseek(fd1, 0, SEEK_SET);
		while(fp>lseek(fd1, 0, SEEK_CUR)){
			read(fd1, &arr1[k1++], n);
		}
		close(fd1);
		
		fp = lseek(fd2, 0, SEEK_END);
		lseek(fd2, 0, SEEK_SET);
		while(fp>lseek(fd2, 0, SEEK_CUR)){
			read(fd2, &arr2[k2++], n);
		}
		close(fd2);

		fp = lseek(fd3, 0, SEEK_END);
		lseek(fd3, 0, SEEK_SET);
		while(fp>lseek(fd3, 0, SEEK_CUR)){
			read(fd3, &arr3[k3++], n);
		}
		close(fd3);

		fp = k1+k2+k3;
		//printf("k1:%d, k2:%d, k3:%d", k1, k2, k3);
		write(sock, &fp, sizeof(fp));
		fp = 1;
		while(k1--){
			write(sock, &arr1[k1].id, sizeof(int));
			write(sock, &fp, sizeof(fp));
			write(sock, &arr1[k1].name, str/2);
			write(sock, &arr1[k1].pass, str);
		}
		fp = 2;
		while(k2--){
			write(sock, &arr2[k2].id, sizeof(int));
			write(sock, &fp, sizeof(fp));
			write(sock, &arr2[k2].name, str/2);
			write(sock, &arr2[k2].pass, str);
		}
		fp = 3;
		while(k3--){
			write(sock, &arr3[k3].id, sizeof(int));
			write(sock, &fp, sizeof(fp));
			write(sock, &arr3[k3].name, str/2);
			write(sock, &arr3[k3].pass, str);
		}

		write(sock,&op_id, sizeof(op_id));
		return op_id;
	}
	if(op_id == 8){
		//view trains
		int fp, entries, k=0;
		struct train arr[100];

		int fd = open(TRAIN, O_RDONLY);

		fp = lseek(fd, 0, SEEK_END);
		lseek(fd, 0, SEEK_SET);
		while(fp>lseek(fd,0,SEEK_CUR)){
			read(fd, &arr[k++], sizeof(struct train));
		}
		close(fd);

		write(sock, &k, sizeof(k));
		while(k--){
			write(sock, &arr[k].tid, sizeof(int));
			write(sock, &arr[k].train_no, sizeof(int));
			write(sock, &arr[k].train_name, sizeof(char)*20);
			write(sock, &arr[k].av_seats, sizeof(int));
		}

		write(sock,&op_id, sizeof(op_id));
		return op_id;
	}
	if(op_id == 9){
		//view bookings
		int fd = open(BOOKING, O_RDONLY);
		struct flock lock;
		lock.l_type = F_RDLCK;
		lock.l_start = 0;
		lock.l_len = 0;
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();
		
		fcntl(fd, F_SETLKW, &lock);

		int fp = lseek(fd, 0, SEEK_END);
		int entries = 0;
		if(fp == 0)
			write(sock, &entries, sizeof(entries));
		else{
			struct bookings bk;
			entries = fp / sizeof(struct bookings);
			//printf("Entries : %d\n", entries);
			write(sock, &entries, sizeof(entries));
			lseek(fd, 0, SEEK_SET);
			while(fp>lseek(fd, 0, SEEK_CUR)){
				read(fd, &bk, sizeof(bk));
				write(sock, &bk.bid, sizeof(bk.bid));
				write(sock, &bk.acc_no, sizeof(bk.acc_no));
				write(sock, &bk.type, sizeof(bk.type));
				write(sock, &bk.trainname, sizeof(bk.trainname));
				write(sock, &bk.seat_start, sizeof(int));
				write(sock, &bk.seat_end, sizeof(int));
				write(sock, &bk.cancelled, sizeof(int));
				//printf("%d\t%d\t%d\t%s\t%d\t%s\t%s\n", bk.bid, bk.seat_start, bk.seat_end, bk.trainname, bk.acc_no, bk.type==0?"USER":"AGENT", bk.cancelled==0?"CONFIRMED":"CANCELLED");
			}
		}
		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);
		close(fd);
		write(sock,&op_id, sizeof(op_id));
		return op_id;
	}
	if(op_id == 10){
		//search users
		int fd1 = open(ACC[0], O_RDONLY);
		int fd2 = open(ACC[1], O_RDONLY);
		int fd3 = open(ACC[2], O_RDONLY);

		int arr1[100], arr2[100], arr3[100];
		int k1=0, k2=0, k3=0, fp;

		char name[10];
		struct account temp;

		read(sock, &name, sizeof(name));

		fp = lseek(fd1, 0, SEEK_END);
		lseek(fd1, 0, SEEK_SET);
		while(fp>lseek(fd1, 0, SEEK_CUR)){
			read(fd1, &temp, sizeof(temp));
			if(!strcmp(temp.name, name))
				arr1[k1++] = temp.id;
		}
		close(fd1);

		fp = lseek(fd2, 0, SEEK_END);
		lseek(fd2, 0, SEEK_SET);
		while(fp>lseek(fd2, 0, SEEK_CUR)){
			read(fd2, &temp, sizeof(temp));
			if(!strcmp(temp.name, name))
				arr2[k2++] = temp.id;
		}
		close(fd2);

		fp = lseek(fd3, 0, SEEK_END);
		lseek(fd3, 0, SEEK_SET);
		while(fp>lseek(fd3, 0, SEEK_CUR)){
			read(fd3, &temp, sizeof(temp));
			if(!strcmp(temp.name, name))
				arr3[k3++] = temp.id;
		}
		close(fd3);

		fp = k1+k2+k3;
		//printf("k1:%d, k2:%d, k3:%d", k1, k2, k3);
		write(sock, &fp, sizeof(fp));
		fp = 1;
		while(k1--){
			write(sock, &arr1[k1], sizeof(int));
			write(sock, &fp, sizeof(fp));
		}
		fp = 2;
		while(k2--){
			write(sock, &arr2[k2], sizeof(int));
			write(sock, &fp, sizeof(fp));
		}
		fp = 3;
		while(k3--){
			write(sock, &arr3[k3], sizeof(int));
			write(sock, &fp, sizeof(fp));
		}

		write(sock,&op_id, sizeof(op_id));
		return op_id;
	}
	if(op_id == 11){
		//search trains
		
		int fp, entries, arrid[100], tno[100], seats[100], k=0;

		char name[20];
		struct train temp;

		read(sock, &name, sizeof(name));

		int fd = open(TRAIN, O_RDONLY);

		fp = lseek(fd, 0, SEEK_END);
		lseek(fd, 0, SEEK_SET);
		while(fp>lseek(fd,0,SEEK_CUR)){
			read(fd, &temp, sizeof(temp));
			if(!strcmp(temp.train_name, name)){
				arrid[k] = temp.tid;
				seats[k] = temp.av_seats;
				tno[k++] = temp.train_no;
			}
		}
		close(fd);

		write(sock, &k, sizeof(k));
		while(k--){
			write(sock, &arrid[k], sizeof(int));
			write(sock, &tno[k], sizeof(int));
			write(sock, &seats[k], sizeof(int));
		}

		write(sock,&op_id, sizeof(op_id));
		return op_id;
	}
	if(op_id == 12) {
		//logout
		write(sock,&op_id, sizeof(op_id));
		return -1;
	}
}

void view_booking(int sock, int id, int type){
	int fd = open(BOOKING, O_RDONLY);
	struct flock lock;
	lock.l_type = F_RDLCK;
	lock.l_start = 0;
	lock.l_len = 0;
	lock.l_whence = SEEK_SET;
	lock.l_pid = getpid();
	
	fcntl(fd, F_SETLKW, &lock);

	int fp = lseek(fd, 0, SEEK_END);
	int entries = 0;
	if(fp == 0)
		write(sock, &entries, sizeof(entries));
	else{
		struct bookings bk[10];
		while(fp>0 && entries<10){
			struct bookings temp;
			fp = lseek(fd, -1*sizeof(struct bookings), SEEK_CUR);
			read(fd, &temp, sizeof(struct bookings));
			if(temp.acc_no == id && temp.type == type)
				bk[entries++] = temp;
			fp = lseek(fd, -1*sizeof(struct bookings), SEEK_CUR);
		}
		write(sock, &entries, sizeof(entries));
		for(fp=0;fp<entries;fp++){
			write(sock, &bk[fp].bid, sizeof(bk[fp].bid));
			write(sock, &bk[fp].trainname, sizeof(bk[fp].trainname));
			write(sock, &bk[fp].seat_start, sizeof(int));
			write(sock, &bk[fp].seat_end, sizeof(int));
			write(sock, &bk[fp].cancelled, sizeof(int));
		}
	}
	lock.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &lock);
	close(fd);
}