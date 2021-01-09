#include "common.h"

#define MAXBUF 100
#define NAMESIZE 10
#define MAXSOCKET 10

pthread_mutex_t mutx;
SSL* client_socket[MAXSOCKET];
char* name_socket[MAXSOCKET];
int count = 0;
#define CERTFILE "server.pem"

SSL_CTX* setup_server_ctx(void)
{
	SSL_CTX* ctx;

	ctx = SSL_CTX_new(SSLv23_method());
	if (SSL_CTX_use_certificate_chain_file(ctx, CERTFILE) != 1)
		int_error("Error loading certificate from file");
	if (SSL_CTX_use_PrivateKey_file(ctx, CERTFILE, SSL_FILETYPE_PEM) != 1)
		int_error("Error loading private key from file");
	return ctx;
}


void* send_msg(char* msg, int sender)
{
	int i;
	pthread_mutex_lock(&mutx);
	for (i = 0; i < count; i++)
		if (i != sender)
			SSL_write(client_socket[i], msg, strlen(msg));
	pthread_mutex_unlock(&mutx);
}

void THREAD_CC thread_run(SSL* ssl)
{

#ifndef WIN32
	pthread_detach(pthread_self());
#endif
	if (SSL_accept(ssl) <= 0)
		int_error("Error accepting SSL connection");

	int read_msg = 0, i = 0;
	int user = count - 1;

	char* msg = NULL;

	while (1)
	{
		msg = malloc(sizeof(char) * (MAXBUF + NAMESIZE + 5));
		read_msg = SSL_read(ssl, msg, MAXBUF + NAMESIZE + 5);
		if (read_msg <= 0)
			break;
		msg[strlen(msg)] = 0;

		if (strncmp(msg, "!bye", 5) == 0)
		{
			pthread_mutex_lock(&mutx);
			for (i = 0; i < count; i++)
			{
				if (ssl == client_socket[i])
				{
					while (i < count - 1)
					{
						client_socket[i] = client_socket[i + 1];
						i++;
					}
				}
			}
			count--;
			pthread_mutex_unlock(&mutx);
		}
		else
		{
			send_msg(msg, user);
		}
		msg = NULL;
	}
	SSL_shutdown(ssl);
	SSL_free(ssl);
	return 0;
}

int main()
{
	BIO* acc, *client;
	SSL* ssl;
	SSL_CTX* ctx;
	THREAD_TYPE tid;
	pthread_t client_thread;

	pthread_mutex_init(&client_thread, NULL);

	init_OpenSSL();
	seed_prng();

	ctx = setup_server_ctx();

	acc = BIO_new_accept(PORT);
	if (!acc)
		int_error("Error creating server socket");
	if (BIO_do_accept(acc) <= 0)
		int_error("Error binding server socket");

	while (1)
	{
		if (BIO_do_accept(acc) <= 0)
			int_error("Error accepting connection");

		client = BIO_pop(acc);
		if (!(ssl = SSL_new(ctx)))
			int_error("Error creating SSL context");

		SSL_set_bio(ssl, client, client);

		pthread_mutex_lock(&mutx);
		client_socket[count] = ssl;
		count++;
		pthread_mutex_unlock(&mutx);
		pthread_create(&client_thread, NULL, thread_run, ssl);
		pthread_detach(client_thread);
	}
	SSL_CTX_free(ctx);
	BIO_free(acc);
	return 0;
}
