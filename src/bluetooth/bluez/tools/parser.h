#define K_BIND 257
#define K_DEVICE 258
#define K_CHANNEL 259
#define K_COMMENT 260
#define K_YES 261
#define K_NO 262
#define NUMBER 263
#define RFCOMM 264
#define STRING 265
#define WORD 266
#define BDADDR 267
typedef union {
	int number;
	char *string;
	bdaddr_t *bdaddr;
} YYSTYPE;
extern YYSTYPE yylval;
