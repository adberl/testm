#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/wait.h> 
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#define READ_END 0
#define WRITE_END 1

//#define DBG

int total_tests;

typedef struct _testcase {
	char name[32];
	char func[32];
	char arg[16];
	char ret[16];
	int end_line;
}	testcase;

void get_tests(FILE*, testcase*);
void find_func(FILE*, testcase*);
void do_test(testcase*, int, int);
int get_total_digits(int);
void print_tests(testcase*);

int main(int argc, char* argv[]) {
	if(argc != 3) {
		printf("usage: %s <python_file> <commands_file>\n", argv[0]);
		return -1;
	}
	int pid;
	int pfd[2];
	int pdb_to_main[2];
	
	if(pipe(pfd) == -1 || pipe(pdb_to_main) == -1) {
		fprintf(stderr, "pipe dead lol\n");
		exit(-1);
	}
	
	pid = fork();
	if(pid < 0) {
		fprintf(stderr, "fork dead lol\n");
		exit(-1);
	}
	
	if(pid == 0) {
		//do child stuff in here
		close(pfd[WRITE_END]);
		close(pdb_to_main[READ_END]);
		// replace STDIN in child with pipe read end (+ pipe2 write end)
		if(dup2(pfd[READ_END], STDIN_FILENO) < 0 ||
				dup2(pdb_to_main[WRITE_END], STDOUT_FILENO) < 0) { 
			perror("couldnt dup2\n");
			exit(2);
		}
		close(pfd[READ_END]);
		close(pdb_to_main[WRITE_END]);

		execlp("python3", "python3", "-m", "pdb", argv[1], NULL);
		perror("couldnt open pdb\n");
		exit(-1);

	} else { // daddy
		close(pfd[READ_END]);
		close(pdb_to_main[WRITE_END]);
		
		FILE* fp;
		if((fp = fopen(argv[2], "r")) == NULL) {
			perror("couldnt open file");
			exit(-1);
		}
		
		fscanf(fp, "%i", &total_tests);
		testcase* test1;
		test1 = (testcase *) malloc(sizeof(testcase) * total_tests);
		get_tests(fp, test1);
		
		// open testfile and look for func
		FILE* tested;
		if((tested = fopen("print_test.py", "r")) == NULL) {
			perror("coulnt open file");
			exit(-1);
		}

		find_func(tested, test1);
		//print_tests(test1);				
		do_test(test1, pfd[WRITE_END], pdb_to_main[READ_END]);
		// end testfile opening

		close(pfd[WRITE_END]);
		wait(NULL);
		close(pdb_to_main[READ_END]);
		fclose(fp);
		fclose(tested);
	}
	printf("END\n");
	return 0;
}

void get_tests(FILE* fp, testcase* test) {

	for(int i = 0; i < total_tests; i++)
		fscanf(fp, "%s %s %s %s", 
			(test+i)->name, (test+i)->func, (test+i)->arg, (test+i)->ret);
}

void find_func(FILE* fp, testcase* test) {

	int lineno = 0;
	for(int i = 0; i < total_tests; i++) {
		char start_line[64] = "def ";
		int found = 0;
		strcat(start_line, (test+i)->func);

		char currentline[128];

		while(fgets(currentline, sizeof(currentline), fp) != NULL) {
			lineno++;
			if(strstr(currentline, start_line) != NULL) {
				while(fgets(currentline, sizeof(currentline), fp) != NULL) {
					if(currentline[0] != '\t') {
						(test+i)->end_line = lineno;
						found = 1;
						break;
					}
					lineno++;
				}
			}
			if(found) break;
		}		
		if(!found) (test+i)->end_line = -1;
	}	

}

void do_test(testcase *test, int pfd, int frompdb) {
	// 1. go until end_line (unt end_line)	
	// 2. step once to step past return (s) (optional)
	// 3. eval
	// 4. print query.
	
	for(int i = 0; i < total_tests; i++) {
		char query[256];
		sprintf(query, "unt %i\n", (test+i)->end_line);
//		printf("\n-----------\nQUERY: %s%i\n-------------\n", query, get_total_digits((test+i)->end_line));
		write(pfd, query, 5+get_total_digits((test+i)->end_line));
		sprintf(query, "!print('[test] %s', 'passed' if %s(%s) == %s else ('failed' + ' expected %s; got ' + str(%s(%s))))\n", (test+i)->name, (test+i)->func, (test+i)->arg, (test+i)->ret, (test+i)->ret, (test+i)->func, (test+i)->arg);
//		printf("\n-----------\nQUERY: %s\n-------------\n", query);
		write(pfd, query, strlen(query));	
		write(pfd, "restart\n", 8); 
		
	}
	
	write(pfd, "q\n", 2);
	
	FILE *f;
	if((f = fdopen(frompdb, "r")) == NULL) {
		perror("coulnt open file");
		exit(-1);
	}		

	char line[256];
	while(fgets(line, sizeof(line), f) != NULL) {
		char tmp[64];
		snprintf(tmp, 64, "(Pdb) [test]"); 
		if(strstr(line, tmp) != NULL)
			printf("%s", &line[6]);
	}
}

int get_total_digits(int a) {

	int all = 0;
	while(a != 0) {
		a = a/10;
		all++;
	}
	
	return all;
}

void print_tests(testcase* test) {

	for(int i = 0; i < total_tests; i++)
		printf("%s:\t%s\t%s\t%s\t%i\n", (test+i)->name,
			(test+i)->func, (test+i)->arg, (test+i)->ret, (test+i)->end_line);
}
