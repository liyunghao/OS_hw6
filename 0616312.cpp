#define FUSE_USE_VERSION 30
#include <fuse.h>
#include <string.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <algorithm>
#define pb push_back
using namespace std;

struct field {
	string name;
	uid_t uid;
	gid_t gid;
	time_t mtime;
	mode_t mode;
	off_t size;
	bool isDir;
};
vector<field> files;

struct tarHeader {
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mTime[12];
	char checksum[8];
	char flag;
	char Lname[100];
	char magic[8];
	char uname[32];
	char gname[32];
	char majorId[8];
	char minorId[8];
	char padding[167];
}header;

void parse(string path, string &tmp1, string &tmp2) {
	tmp1.clear();
	tmp2.clear();
	tmp2.pb(path.length()-1);
	int flag = 0;
	for (int i = path.length()-2; i >= 0; i--) {
		if (path[i] == '/') flag = 1;
		if (flag == 0) tmp2.pb(path[i]);
		else tmp1.pb(path[i]);
	}
	reverse(tmp1.begin(), tmp1.end());
	reverse(tmp2.begin(), tmp2.end());	
	cout << "tmp1 = " << tmp1 << '\n';	
	cout << "tmp2 = " << tmp2 << '\n';
	return;
}
int my_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
	(void) offset;
	(void) fi;
	string Path = string(path);
	filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);
	cout << "Path = "<< Path << '\n';
		
	for (int i = 0; i < files.size(); i++) {
		string pre, post;
		parse(files[i].name, pre, post);
		cout << "file name = " << files[i].name << '\n';
		cout << "pre = " << pre << '\n';
		if (pre == Path || pre == Path + '/'){
			if (post[post.length()-1] == '/') post.resize(post.length()-1);
			cout << "fill " << post << '\n'; 
			filler(buffer, post.c_str(), NULL, 0);
		}
	}
	return 0;
}

int my_getattr(const char *path, struct stat *st) {
	memset(st, 0, sizeof(st));
	cout << "getattr\n";
	string Path = string (path);
	if (strcmp(path, "/") == 0) {
		st->st_mode = S_IFDIR | 0444;
		st->st_mtime = time(NULL);
		st->st_uid = getuid();
		st->st_gid = getgid();
		st->st_size = 0;
		return 0;
	} else {
		for (int i = 0; i < files.size(); i++) {
			struct field tmp = files[i];	
			if (tmp.name == Path || tmp.name == (Path + '/')) {
				if (tmp.isDir) st->st_mode = S_IFDIR | tmp.mode;
				else st->st_mode = S_IFREG | tmp.mode;
				st->st_size = tmp.size;
				st->st_uid = getuid();
				st->st_gid = tmp.gid;
				st->st_mtime = tmp.mtime;
				return 0;
			}
		}
	}
	return -ENOENT;
}

int my_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	return 0;
}

static struct fuse_operations op;

int octtodec(char *input) {
	int sz = strlen(input);
	
	int dec = 0;
	for (int i = 0; i < sz; i++) {
		dec *= 8;
		dec += (input[i] - '0');
	}
	return dec;
}
void parseTar() {
	FILE *fp;
	fp = fopen("test.tar", "rb");
	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	if (size % 512) {
		cerr << "tar file size error\n";
		return;
	}
	int pos = 0;
	while (1) {
		memset(&header, 0, sizeof(header));
		int read_size = fread(&header, 512, 1, fp);
		if (read_size != 1) break;
		pos += 512;
		int blockCnt = 0, file_size = 0;
		file_size = octtodec(header.size);
		blockCnt = (file_size + 511) / 512;
		struct field tmp;
		if (strlen(header.name)) {
			tmp.name = header.name;
			tmp.uid = octtodec(header.uid);
			tmp.gid = octtodec(header.gid);
			tmp.mtime = octtodec(header.mTime);
			tmp.size = file_size;
			tmp.mode = octtodec(header.mode);
			tmp.isDir = header.flag == 0 ? 0 : 1;
			files.push_back(tmp);
			cout << tmp.name << '\t' << header.flag << '\n';
		}
		pos += blockCnt * 512;
		fseek(fp, pos, SEEK_SET);

	}
}
int main(int argc, char **argv) {
	memset(&op, 0, sizeof(op));
	op.getattr = my_getattr;
	op.readdir = my_readdir;
	op.read = my_read;
	//struct tarHeader header;
	//memset(&header, 0, sizeof(header));
	//int read_size = fread(&header, 512, 1, fp);
	//cout << read_size << '\n';
	//cout << header.name << '\n' << octtodec(header.uid) << '\n';
	parseTar();
	//for (int i = 0; i < files.size(); i++) {
	//	cout << files[i].name << '\t' << files[i].name.size() << '\n';
	//}
	//cout << sizeof(header) << '\n';
	return fuse_main(argc, argv, &op, NULL);
}
