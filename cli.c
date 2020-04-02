#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#define PORT 8090
#define PASS_LENGTH 20

int irctc(int sock);
int menu2(int sock, int type);
int do_admin_action(int sock, int action);
int do_action(int sock, int opt);
void view_booking(int sock, int all);

int main(int argc, char * argv[]){
	char *ip = "127.0.0.1";
	if(argc==2){
		ip = argv[1];
	}
	int cli_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(cli_fd == -1){
		printf("socket creation failed\n");
		exit(0);
	}
	struct sockaddr_in ca;
	ca.sin_family=AF_INET;
	ca.sin_port= htons(PORT);
	ca.sin_addr.s_addr = inet_addr(ip);
	if(connect(cli_fd, (struct sockaddr *)&ca, sizeof(ca))==-1){
		printf("connect failed\n");
		exit(0);
	}
	printf("connection established\n");
	
	while(irctc(cli_fd)!=3);
	close(cli_fd);

	return 0;
}

int irctc(int sock){
	int opt;
	char buf[100];
	system("clear");
	printf("+++++++TRAIN BOOKING++++++++\n");
	printf("1. Sign In\n");
	printf("2. Sign Up\n");
	printf("3. Exit\n");
	printf("+++++Enter Your Choice++++++\n");
	scanf("%s", buf);
	opt = atoi(buf);
	if(opt==0  || opt<1 || opt>3){
		printf("Invalid choice. Please try again..\nPress [Enter] to continue...\n");
		while(getchar()!='\n');
		getchar();
		return 0;
	}
	write(sock, &opt, sizeof(opt));
	if(opt==1){
		int type, acc_no;
		char password[PASS_LENGTH];
		printf("Enter the type of account:\n");
		printf("1. Customer\n2. Agent\n3. Admin\n");
		printf("Your Response: ");
		scanf("%s", buf);
		type = atoi(buf);
		if(type==0 || type<1 || type>3){
			printf("Invalid choice. Please try again..\nPress [Enter] to continue...\n");
			while(getchar()!='\n');
			getchar();
			return 0;
		}
		printf("Enter Your Account Number: ");
		scanf("%s", buf);
		acc_no = atoi(buf);
		if(acc_no==0){
			printf("Invalid Account Number Format. Please try again..\nPress [Enter] to continue...\n");
			while(getchar()!='\n');
			getchar();
			return 0;
		}
		memset(password, 0, PASS_LENGTH);

		LBL:

		strcpy(password,getpass("Enter the password: "));

		if(strlen(password)<=0) goto LBL;

		write(sock, &type, sizeof(type));
		write(sock, &acc_no, sizeof(acc_no));
		write(sock, &password, strlen(password));

		int valid_login;
		read(sock, &valid_login, sizeof(valid_login));
		if(valid_login == 1){
			while(menu2(sock, type)!=-1);
			system("clear");
			return 1;
		}
		else{
			printf("Login Failed\n");
			while(getchar()!='\n');
			getchar();
			return 1;
		}
	}
	else if(opt==2){
		int type, acc_no;
		char password[PASS_LENGTH], secret_pin[5], name[10];
		printf("Enter the type of account:\n");
		printf("1. Customer\n2. Agent\n3. Admin\n");
		printf("Your Response: ");
		scanf("%s", buf);
		type = atoi(buf);
		if(type==0 || type<1 || type>3){
			printf("Invalid choice. Please try again..\nPress [Enter] to continue...\n");
			while(getchar()!='\n');
			getchar();
			return 0;
		}
		printf("Enter your name: ");scanf("%s", name);
		memset(password, 0, PASS_LENGTH);
		
		LBL2:
		
		strcpy(password,getpass("(Password must contain 1 character and 1 number, else account will not login.)\nEnter the password: "));
		
		if(strlen(password)<=0) goto LBL2;

		printf("%s\n", password);
		if(type == 3){
			int attempt = 1;
			while(1){
			
				strcpy(secret_pin, getpass("Enter secret PIN to create ADMIN account: "));attempt++;
				if(strcmp(secret_pin, "root")!=0 && attempt<=3) printf("Invalid PIN. Please try again.\n");
				else break;
			}
			if(!strcmp(secret_pin, "root"));
			else {
				printf("Too many attempts.\nExiting...\n");
				exit(0);
			}
		}
		write(sock, &type, sizeof(type));
		write(sock, &name, sizeof(name));
		password[strlen(password)]='\0';
		write(sock, &password, PASS_LENGTH);

		read(sock, &acc_no, sizeof(acc_no));
		printf("Remember the account no of further login: %d\n", acc_no);
		while(getchar()!='\n');
		getchar();		
		return 2;
	}
	else
		return 3;
}

int menu2(int sock, int type){
	int opt = 0;
	char buf[100];
	jump:
	if(type == 1 || type == 2){
		system("clear");
		printf("++++ OPTIONS ++++\n");
		printf("1. BOOK TICKET\n");
		printf("2. View Bookings\n");
		printf("3. Update Booking\n");
		printf("4. Cancel booking\n");
		printf("5. Logout\n");
		printf("Your Choice: ");
		scanf("%s", buf);
		opt = atoi(buf);
		if(opt==0 || opt<1 || opt>5){
			printf("Invalid choice. Please try again..\nPress [Enter] to continue...\n");
			while(getchar()!='\n');
			getchar();
			goto jump;
		}
		return do_action(sock, opt);
	}
	else{
		system("clear");
		printf("++++ OPTIONS ++++\n");
		printf("1.  Add Train\n");
		printf("2.  Delete Train\n");
		printf("3.  Modify Train\n");
		printf("4.  Add User\n");
		printf("5.  Delete User\n");
		printf("6.  Modify User\n");
		printf("7.  View All Users\n");
		printf("8.  View All Trains\n");
		printf("9.  View All Bookings\n");
		printf("10. Search User\n");
		printf("11. Search Train\n");
		printf("12. Logout\n");
		printf("Your Choice: ");
		scanf("%s", buf);
		opt = atoi(buf);
		if(opt==0 || opt<1 || opt>12){
			printf("Invalid choice. Please try again..\nPress [Enter] to continue...\n");
			while(getchar()!='\n');
			getchar();
			goto jump;
		}
		return do_admin_action(sock, opt);
	}
}

int do_admin_action(int sock, int opt){
	switch(opt){
		case 1:{
			int tno;
			char tname[20];
			write(sock, &opt, sizeof(opt));
			printf("Enter Train Name: ");scanf("%s", tname);
			printf("Enter Train No. : ");scanf("%d", &tno);
			write(sock, &tname, sizeof(tname));
			write(sock, &tno, sizeof(tno));
			read(sock, &opt, sizeof(opt));
			if(opt == 1 ) printf("Train Added Successfully.\n");
			while(getchar()!='\n');
			getchar();
			return opt;
			break;
		}
		case 2:{
			int no_of_trains;
			write(sock, &opt, sizeof(opt));
			read(sock, &no_of_trains, sizeof(int));
			while(no_of_trains>0){
				int tid, tno;
				char tname[20];
				read(sock, &tid, sizeof(tid));
				read(sock, &tname, sizeof(tname));
				read(sock, &tno, sizeof(tno));
				if(!strcmp(tname, "deleted"));else
				printf("%d.\t%d\t%s\n", tid+1, tno, tname);
				no_of_trains--;
			}
			printf("Enter 0 to cancel.\nEnter the train ID to delete: "); scanf("%d", &no_of_trains);
			write(sock, &no_of_trains, sizeof(int));
			read(sock, &opt, sizeof(opt));
			if(opt == no_of_trains && opt) printf("Train deleted successfully\n");
			while(getchar()!='\n');
			getchar();
			return opt;
			break;
		}
		case 3:{
			int no_of_trains;
			write(sock, &opt, sizeof(opt));
			read(sock, &no_of_trains, sizeof(int));
			while(no_of_trains>0){
				int tid, tno;
				char tname[20];
				read(sock, &tid, sizeof(tid));
				read(sock, &tname, sizeof(tname));
				read(sock, &tno, sizeof(tno));
				if(!strcmp(tname, "deleted"));else
				printf("%d.\t%d\t%s\n", tid+1, tno, tname);
				no_of_trains--;
			}
			printf("Enter 0 to cancel.\nEnter the train ID to modify: "); scanf("%d", &no_of_trains);
			write(sock, &no_of_trains, sizeof(int));
			printf("What parameter do you want to modify?\n1. Train No.\n2. Available Seats\n");
			printf("Your Choice: ");scanf("%d", &no_of_trains);
			write(sock, &no_of_trains, sizeof(int));
			read(sock, &no_of_trains, sizeof(int));
			printf("Current Value: %d\n", no_of_trains);				
			printf("Enter Value: ");scanf("%d", &no_of_trains);
			write(sock, &no_of_trains, sizeof(int));
			
			read(sock, &opt, sizeof(opt));
			if(opt == 3) printf("Train Data Modified Successfully\n");
			while(getchar()!='\n');
			getchar();
			return opt;
			break;
		}
		case 4:{
			write(sock, &opt, sizeof(opt));
			char pass[PASS_LENGTH],name[10], buf[100];
			jump:
			printf("Enter the type of account\n1. User\n2.Agent\n3.Admin\nYour Choice:");
			scanf("%s", buf);
			int type = atoi(buf);
			if(type==0 || type<1 || type>3){
				printf("Invalid Account Type. Try Again...");
				while(getchar()!='\n');
				getchar();
				goto jump;
			}
			write(sock, &type, sizeof(type));
			printf("Enter the name: ");scanf("%s", name);

			LBL3:
			
			strcpy(pass, getpass("Enter a password"));

			if(strlen(pass)<=0) goto LBL3;

			write(sock, &name, sizeof(name));
			write(sock, &pass, sizeof(pass));
			read(sock, &opt, sizeof(opt));
			printf("The Account Number for this %s is: %d\n", type==1?"USER":type==2?"AGENT":"ADMIN",opt);
			read(sock, &opt, sizeof(opt));
			if(opt == 4)printf("Successfully created %s account\n", type==1?"USER":type==2?"AGENT":"ADMIN");
			while(getchar()!='\n');
			getchar();
			return opt;
			break;
		}
		case 5: {
			int choice, users, id;
			write(sock, &opt, sizeof(opt));
			printf("What kind of account do you want to delete?\n");
			printf("1. Customer\n2. Agent\n3. Admin\n");
			printf("Your Choice: ");
			scanf("%d", &choice);
			write(sock, &choice, sizeof(choice));
			read(sock, &users, sizeof(users));
			while(users--){
				char name[10];
				read(sock, &id, sizeof(id));
				read(sock, &name, sizeof(name));
				if(strcmp(name, "deleted")!=0)
				printf("%d\t%s\n", id, name);
			}
			printf("Enter the ID to delete: ");scanf("%d", &id);
			write(sock, &id, sizeof(id));
			read(sock, &opt, sizeof(opt));
			if(opt == 5) printf("Successfully deleted user\n");
			while(getchar()!='\n');
			getchar();
			return opt;
		}
		case 6: {
			int choice, users, id;
			char name[10], pass[20], buf[100];
			write(sock, &opt, sizeof(opt));
			jump3:
			printf("What kind of account do you want to update?\n");
			printf("1. Customer\n2. Agent\n3. Admin\n");
			printf("Your Choice: ");
			scanf("%s", buf);
			choice = atoi(buf);
			if(choice==0 || choice<1 || choice>3){
				printf("Invalid Choice. Please try again...\n");
				while(getchar()!='\n');
				getchar();
				goto jump3;
			}
			write(sock, &choice, sizeof(choice));
			read(sock, &users, sizeof(users));
			while(users--){
				read(sock, &id, sizeof(id));
				read(sock, &name, sizeof(name));
				if(strcmp(name, "deleted")!=0)
				printf("%d\t%s\n", id, name);
			}
			jump2:
			printf("Enter the ID to update: ");
			scanf("%s", buf);
			id = atoi(buf);
			if(id==0){
				printf("Invalid Account ID. Please try again...\n");
				while(getchar()!='\n');
				getchar();
				goto jump2;
			}			
			write(sock, &id, sizeof(id));
			read(sock,&name,sizeof(name));
			read(sock,&pass,sizeof(pass));
			printf("What data do you want to update:\n");
			printf("1. Users Name\n2. Users Password\nYour Choice: ");
			scanf("%d", &id);
			if(id == 1){
				printf("Current Name = %s\n", name);
				printf("New Name     = ");
				scanf("%s", name);
			}
			else if(id == 2){
				printf("Current Password = %s\n", pass);
				printf("New Password     = ");
				scanf("%s", pass);
			}
			write(sock, &id, sizeof(id));
			if(id==1)
			write(sock, &name, sizeof(name));
			if(id==2)
			write(sock, &pass, sizeof(pass));
			
			read(sock, &opt, sizeof(opt));
			if(opt == 6)
			printf("Updated Successfully\n");
			while(getchar()!='\n');
			getchar();
			return opt;
			break;
		}
		case 7:{
			//view users
			write(sock, &opt, sizeof(opt));
			int entries;
			read(sock, &entries, sizeof(entries));

			if(entries<=0)
			printf("No user found\n");
			else
			while(entries--){
				int id, type;
				char name[10], password[20];
				read(sock, &id, sizeof(id));
				read(sock, &type, sizeof(type));
				read(sock, &name, sizeof(name));
				read(sock, &password, sizeof(password));
				if(strcmp(name, "deleted")!=0)
				printf("ID:%d\tTYPE:%s\tNAME:%s\tPASS:%s\n", id, type==1?"USER ":type==2?"AGENT":"ADMIN", name, password);
			}
			read(sock, &opt, sizeof(opt));
			while(getchar()!='\n');
			getchar();
			return opt;
			break;
		}
		case 8:{
			//view trains
			write(sock, &opt, sizeof(opt));
			int entries;
			read(sock, &entries, sizeof(entries));

			if(entries<=0)
			printf("No train found\n");
			else
			while(entries--)
			{
				int id, tno, av_seats;
				char name[20];
				read(sock, &id, sizeof(id));
				read(sock, &tno, sizeof(tno));
				read(sock, &name, sizeof(name));
				read(sock, &av_seats, sizeof(av_seats));
				if(strcmp(name, "deleted")!=0)
				printf("ID: %d\tTRAIN NO: %d\tNAME: %s\tAVAILABLE SEATS: %d\n", id, tno, name, av_seats);
			}

			read(sock, &opt, sizeof(opt));
			while(getchar()!='\n');
			getchar();
			return opt;
			break;
		}
		case 9:{
			//view bookings
			write(sock, &opt, sizeof(opt));
			view_booking(sock, 1);
			read(sock, &opt, sizeof(opt));
			return opt;
			break;
		}
		case 10:{
			//search users
			write(sock, &opt, sizeof(opt));
			char name[10];
			int entries;
			printf("Enter the name of user to search\n");
			scanf("%s", name);
			
			write(sock, &name, sizeof(name));

			read(sock, &entries, sizeof(entries));

			if(entries<=0)
			printf("No user found\n");
			else
			while(entries--){
				int id, type;
				read(sock, &id, sizeof(id));
				read(sock, &type, sizeof(type));
				printf("ID: %d\tTYPE: %d\n", id, type);
			}

			read(sock, &opt, sizeof(opt));
			while(getchar()!='\n');
			getchar();
			return opt;
			break;
		}
		case 11:{
			//search trains
			write(sock, &opt, sizeof(opt));
			
			char name[20];
			int entries;
			printf("Enter the name of train to search: \n");
			scanf("%s", name);
			
			write(sock, &name, sizeof(name));

			read(sock, &entries, sizeof(entries));

			if(entries<=0)
			printf("No train found\n");
			else
			while(entries--)
			{
				int id, type, av_seats;
				read(sock, &id, sizeof(id));
				read(sock, &type, sizeof(type));
				read(sock, &av_seats, sizeof(av_seats));
				printf("ID: %d\tTRAIN NO: %d\tAVAILABLE SEATS: %d\n", id, type, av_seats);
			}

			read(sock, &opt, sizeof(opt));
			while(getchar()!='\n');
			getchar();
			return opt;
			break;
		}
		case 12:{
			write(sock, &opt, sizeof(opt));
			read(sock, &opt, sizeof(opt));
			if(opt==12) printf("Logged out successfully.\n");
			while(getchar()!='\n');
			getchar();
			return -1;
			break;
		}
		default: return -1;
	}
}

int do_action(int sock, int opt){
	switch(opt){
		case 1:{
			//book a ticket
			int trains, trainid, trainavseats, trainno, required_seats=0;
			char trainname[20];
			write(sock, &opt, sizeof(opt));
			read(sock, &trains, sizeof(trains));
			printf("ID\tT_NO\tAV_SEAT\tTRAIN NAME\n");
			while(trains--){
				read(sock, &trainid, sizeof(trainid));
				read(sock, &trainno, sizeof(trainno));
				read(sock, &trainavseats, sizeof(trainavseats));
				read(sock, &trainname, sizeof(trainname));
				if(strcmp(trainname, "deleted")!=0)
				printf("%d\t%d\t%d\t%s\n", trainid, trainno, trainavseats, trainname);
			}
			printf("Enter the train ID: "); scanf("%d", &trainid);
			write(sock, &trainid, sizeof(trainid));
			read(sock, &trainavseats, sizeof(trainavseats));
			if(trainavseats>0)
			{printf("Enter the number of seats: "); scanf("%d", &required_seats);}
			if(trainavseats>=required_seats && required_seats>0)
				write(sock, &required_seats, sizeof(required_seats));
			else{
				required_seats = -1;
				write(sock, &required_seats, sizeof(required_seats));
			}
			read(sock, &opt, sizeof(opt));
			
			if(opt == 1) printf("Tickets booked successfully\n");
			else printf("Tickets were not booked. Please try again.\n");
			printf("Press any key to continue...\n");
			while(getchar()!='\n');
			getchar();
			while(!getchar());
			return 1;
		}
		case 2:{
			//View your bookings
			write(sock, &opt, sizeof(opt));
			view_booking(sock, 0);
			read(sock, &opt, sizeof(opt));
			return 2;
		}
		case 3:{
			//update bookings
			int val;
			write(sock, &opt, sizeof(opt));
			view_booking(sock, 0);
			printf("Enter the booking id to be updated: "); scanf("%d", &val);
			write(sock, &val, sizeof(int));	//Booking ID
			printf("What information do you want to update:\n1.Increase No of Seats\n2. Decrease No of Seats\nYour Choice: ");
			scanf("%d", &val);
			write(sock, &val, sizeof(int));	//Increase or Decrease
			if(val == 1){
				printf("How many tickets do you want to increase: \n");scanf("%d",&val);
				write(sock, &val, sizeof(int));	//No of Seats
			}else if(val == 2){
				printf("How many tickets do you want to decrease: \n");scanf("%d",&val);
				write(sock, &val, sizeof(int));	//No of Seats		
			}
			read(sock, &opt, sizeof(opt));
			if(opt == -2)
				printf("Operation failed. No more available seats\n");
			else printf("Operation succeded.\n");
			while(getchar()!='\n');
			getchar();
			return 3;
		}
		case 4: {
			//cancel booking
			int val;
			write(sock, &opt, sizeof(opt));
			view_booking(sock, 0);
			printf("Enter the booking id to be cancelled: "); scanf("%d", &val);
			write(sock, &val, sizeof(int));	//Booking ID
			read(sock, &opt, sizeof(opt));
			if(opt==4)
				printf("Operation succeded.\n");
			else
				printf("Operation failed. Try again.\n");
			break;
		}
		case 5: {
			write(sock, &opt, sizeof(opt));
			read(sock, &opt, sizeof(opt));
			if(opt == 5) printf("Logged out successfully.\n");
			while(getchar()!='\n');
			getchar();
			return -1;
			break;
		}
		default: return -1;
	}
}

void view_booking(int sock, int all){
	int entries;
	read(sock, &entries, sizeof(int));
	if(entries == 0) printf("No records found.\n");
	else if(all)
	printf("+++++Bookings Data+++++\n");
	else printf("Your recent bookings are... :\n");
	while(entries--){
		int bid, bks_seat, bke_seat, cancelled, uid, utype;
		char trainname[20];
		read(sock,&bid, sizeof(bid));
		if(all==1){
			read(sock,&uid, sizeof(uid));
			read(sock,&utype, sizeof(utype));
		}
		read(sock,&trainname, sizeof(trainname));
		read(sock,&bks_seat, sizeof(int));
		read(sock,&bke_seat, sizeof(int));
		read(sock,&cancelled, sizeof(int));
		if(!cancelled && !all)
		printf("BookingID: %d\t1st Ticket: %d\tLast Ticket: %d\tTRAIN :%s\n", bid, bks_seat, bke_seat, trainname);
		else if(all)
		printf("%d\t%d\t%d\t%s\t%d\t%s\t%s\n", bid, bks_seat, bke_seat, trainname, uid, utype==1?"USER":"AGENT", cancelled==0?"CONFIRMED":"CANCELLED");		
	}
	printf("Press [Enter] key to continue...\n");
	while(getchar()!='\n');
	getchar();
}