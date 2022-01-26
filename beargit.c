#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/stat.h>

#include "beargit.h"
#include "util.h"

/* Implementation Notes:
 *
 * - Functions return 0 if successful, 1 if there is an error.
 * - All error conditions in the function description need to be implemented
 *   and written to stderr. We catch some additional errors for you in main.c.
 * - Output to stdout needs to be exactly as specified in the function description.
 * - Only edit this file (beargit.c)
 * - You are given the following helper functions:
 *   * fs_mkdir(dirname): create directory <dirname>
 *   * fs_rm(filename): delete file <filename>
 *   * fs_mv(src,dst): move file <src> to <dst>, overwriting <dst> if it exists
 *   * fs_cp(src,dst): copy file <src> to <dst>, overwriting <dst> if it exists
 *   * write_string_to_file(filename,str): write <str> to filename (overwriting contents)
 *   * read_string_from_file(filename,str,size): read a string of at most <size> (incl.
 *     NULL character) from file <filename> and store it into <str>. Note that <str>
 *     needs to be large enough to hold that string.
 *  - You NEED to test your code. The autograder we provide does not contain the
 *    full set of tests that we will run on your code. See "Step 5" in the homework spec.
 */

int beargit_init(void) {
  fs_mkdir(".beargit");

  FILE* findex = fopen(".beargit/.index", "w");
  fclose(findex);
  
  write_string_to_file(".beargit/.prev", "0000000000000000000000000000000000000000");

  return 0;
}


int beargit_add(const char* filename) {
  FILE* findex = fopen(".beargit/.index", "r");			//opens the file index 
  FILE* fnewindex = fopen(".beargit/.newindex", "w");		//fopen creates a new empty file
								//for output operations
  char line[FILENAME_SIZE];					//line is an array of characters storing name
  while (fgets(line, sizeof(line), findex)) {	//reads character from findex (file of names) and stores as 
  						//a string in char array line
    strtok(line, "\n");
    if (strcmp(line, filename) == 0) {
      fprintf(stdout, "ERROR: File %s already added\n", filename);
      fclose(findex);
      fclose(fnewindex);
      fs_rm(".beargit/.newindex");
      return 3;
    }

    fprintf(fnewindex, "%s\n", line);
  }

  fprintf(fnewindex, "%s\n", filename);
  fclose(findex);
  fclose(fnewindex);

  fs_mv(".beargit/.newindex", ".beargit/.index");

  return 0;
}


int beargit_rm(const char* filename) {
  FILE* oldindex = fopen(".beargit/.index", "r");
  FILE* newindex = fopen(".beargit/.newindex", "w");
  char filename_line[FILENAME_SIZE];
  int found = 0;
  while (fgets(filename_line, sizeof(filename_line), oldindex)) {
  	strtok(filename_line, "\n");
  	if (strcmp(filename_line, filename) == 0) {
  		found = 1;
  	}
  	else {
  		fprintf(newindex, "%s\n", filename_line);
  	}
  }
  if (!found) {
  	fprintf(stdout, "ERROR: File %s not tracked\n", filename);
		fclose(oldindex);
    fclose(newindex);
    fs_rm(".beargit/.newindex");
    return 1;
  }
  fclose(oldindex);
  fclose(newindex);
  fs_mv(".beargit/.newindex", ".beargit/.index");
  return 0;
}

const char* go_bears = "GO BEARS!";

int is_commit_msg_ok(const char* msg) {
	int n = strlen(msg);
	int len = strlen(go_bears);
	int i = 0, k = 0, count = 0;
	while (i < n) {
		if (msg[i] == 'G') {
			k = 0;
			count = 0;
			while (i < n && k < len) {
				if (msg[i] != go_bears[k]) {
					break;
				}
				count++;
				i++;
				k++;
			}
		}
		i++;
	}
	if (count == len) return 1;
	else return 0;
}

void next_commit_id(char* commit_id) {
  if (commit_id[0] == '0') {
  	int i = 0;
  	for (i = 0; commit_id[i] != '\0'; i++) {
  		commit_id[i] = '6';
  	}
  }
  else {
  	int inc = 0;
  	int i = 0;
  	while (inc != 1) {
  		if (commit_id[i] == '6') {
  			commit_id[i] = '1';
  			inc++;
  		}
  		else if (commit_id[i] == '1') {
  			commit_id[i] = 'c';
  			inc++;
  		}
  		else {
  			i++;
  		}
  	}
  }
}

void beargit_move_file(char* dest, char* file, char* file_src) {
	int dest_new_len = strlen(dest) + strlen(file) + 1;
	char dest_new[dest_new_len];
	strcpy(dest_new, dest);
	if (file[0] != '/') {
		strcat(dest_new, "/");
	}
	strcat(dest_new, file);
	fs_cp(file_src, dest_new);
}


int beargit_commit(const char* msg) {
  if (!is_commit_msg_ok(msg)) {
    fprintf(stderr, "ERROR: Message must contain \"%s\"\n", go_bears);
    return 1;
  }

  char commit_id[COMMIT_ID_SIZE];
  read_string_from_file(".beargit/.prev", commit_id, COMMIT_ID_SIZE);
  next_commit_id(commit_id);

	char new_dir[50];
	strcpy(new_dir, ".beargit/");
	strcat(new_dir, commit_id);
	fs_mkdir(new_dir); //making the new directory with the commit id
	beargit_move_file(new_dir, "/.prev", ".beargit/.prev");		//copying the files to the new dir
	beargit_move_file(new_dir, "/.index", ".beargit/.index");
	char msg_file[50];					//copy msg
	strcpy(msg_file, new_dir);		
	strcat(msg_file, "/.msg");
	write_string_to_file(msg_file, msg);	
	
	FILE* findex = fopen(".beargit/.index", "r");
	char line[FILENAME_SIZE];
	while (fgets(line, FILENAME_SIZE, findex)) {
		strtok(line, "\n");
		beargit_move_file(new_dir, line, line);
	}
	fclose(findex);
	
	write_string_to_file(".beargit/.prev", commit_id);
  return 0;
}

int beargit_status() {
	FILE* index = fopen(".beargit/.index", "r");
	char filename[FILENAME_SIZE];
	int i = 0;
	fprintf(stdout, "Tracked Files:\n\n");
  while (fgets(filename, sizeof(filename), index)) {
  	i++;
  	fprintf(stdout,"      %s\n", filename);
  }
  if (i == 1)
  	fprintf(stdout, "%d file total\n", i);
  else if (i == 0 || i > 1)
    fprintf(stdout, "%d files total\n", i);
  fclose(index);
  return 0;
}

int get_commit_dir(char* commit_id, char* commit_dir) {
	if (strcmp(commit_id, "0000000000000000000000000000000000000000") == 0) {
		return 0;
	}
	char file_path[50];
	strcpy(file_path, ".beargit/");
	strcat(file_path, commit_id);
	strcpy(commit_dir, file_path);
	return 1;
}

int get_commit_msg(char* commit_dir, char* info, char* commit_msg, int SIZE) {
	int file_path_len = 1 + strlen(commit_dir) + strlen(info);
	char file_path[file_path_len];
	strcpy(file_path, commit_dir);
	strcat(file_path, info);
	char file_info[SIZE];
	read_string_from_file(file_path, file_info, SIZE);
	strcpy(commit_msg, file_info);
}
	

int beargit_log() {
  char prev_commit_id[COMMIT_ID_SIZE]; //save commit id in this char array
 	read_string_from_file(".beargit/.prev", prev_commit_id, COMMIT_ID_SIZE);
 	if (strcmp(prev_commit_id, "0000000000000000000000000000000000000000") == 0) {
    fprintf(stderr, "ERROR: There are no commits!\n");
    return 0;
  }
  while (strcmp(prev_commit_id, "0000000000000000000000000000000000000000") != 0) {
  	char commit_dir[50];
		char commit_msg[MSG_SIZE];
		get_commit_dir(prev_commit_id, commit_dir);
		get_commit_msg(commit_dir, "/.msg", commit_msg, MSG_SIZE);
		
		printf("\n");
		printf("commit %s\n", prev_commit_id);
		printf("\t%s", commit_msg);
		get_commit_msg(commit_dir, "/.prev", prev_commit_id, COMMIT_ID_SIZE);
	}
	printf("\n");
	return 0;
}
