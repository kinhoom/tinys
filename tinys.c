/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2017 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_tinys.h"

/* If you declare any globals in php_tinys.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(tinys)
*/

/* True global resources - no need for thread safety here */
//static int le_tinys;

zend_class_entry *tinys_ce;



ZEND_BEGIN_ARG_INFO_EX(arginfo_tinys_server, 0, 0, 1)
    ZEND_ARG_INFO(0, host)
    ZEND_ARG_INFO(0, port)
ZEND_END_ARG_INFO()



ZEND_BEGIN_ARG_INFO_EX(arginfo_tinys_set_on, 0, 0, 1)
    ZEND_ARG_INFO(0, event)
    ZEND_ARG_INFO(0, callback)
ZEND_END_ARG_INFO()

int php_tinys_onReceive(int fd,char *line,int n)
{
	printf("php_tinys_onReceive fd %d \n",fd);
	zval *retval = NULL;
	zval **args[4];

	zval *callback = php_sw_server_callbacks[1];
	zval *method = NULL;
	zval *obj = NULL;

	zval *zfd = NULL;
	zval *zdata;

	SW_MAKE_STD_ZVAL(zfd);
	SW_MAKE_STD_ZVAL(zdata);
//	ZVAL_UNDEF(zfd);
//	ZVAL_LONG(zfd, (long ) fd);
	ZVAL_LONG(zfd,5);
	SW_ZVAL_STRINGL(zdata, line, n, 1);

	printf("php_tinys_onReceive step 0 \n");
//	int type = Z_TYPE_P(callback);
//	printf('callback type %d',type);
//	if (zend_hash_num_elements(Z_ARRVAL_P(callback)) == 2) {
//		printf('get callback \n');
//		obj = zend_hash_index_find(Z_ARRVAL_P(callback), 0);
//		method = zend_hash_index_find(Z_ARRVAL_P(callback), 1);

//		printf("method %s \n",Z_STRING_P(method));
//	}

	if (callback == NULL || ZVAL_IS_NULL(callback))
	{
//		swoole_php_fatal_error(E_WARNING, "onReceive callback is null.");
		printf("onReceive callback is null.");
		return 1;
	}

	char *func_name = NULL;


	if (!sw_zend_is_callable(callback, 0, &func_name TSRMLS_CC))
	{
		printf("Function '%s' is not callable", func_name);
		efree(func_name);
		return 0;
	}

	printf("Function '%s' ", func_name);
	efree(func_name);



	printf("php_tinys_onReceive step 1 \n");

	args[0] = &zfd;
	args[1] = &zdata;
	args[2] = &line;
	args[3] = &line;

	if (sw_call_user_function_ex(EG(function_table), NULL, callback, &retval, 4, args, 0, NULL TSRMLS_CC) == FAILURE)
	{
		printf("onReceive handler error.\n");
	}
}


 int sw_call_user_function_ex(HashTable *function_table, zval** object_pp, zval *function_name, zval **retval_ptr_ptr, uint32_t param_count, zval ***params, int no_separation, HashTable* ymbol_table)
{
	 printf("php_tinys_onReceive step 2 \n");
    zval real_params[SW_PHP_MAX_PARAMS_NUM];	//SW_PHP_MAX_PARAMS_NUM
    int i = 0;
    printf("php_tinys_onReceive step 2.1.1 \n");
    for (; i < param_count; i++)
    {
        real_params[i] = **params[i];
    }
    printf("php_tinys_onReceive step 2.1.2 \n");
    zval phpng_retval;
    *retval_ptr_ptr = &phpng_retval;
    zval *object_p = (object_pp == NULL) ? NULL : *object_pp;
    printf("php_tinys_onReceive step 2.2 \n");
    return call_user_function_ex(function_table, object_p, function_name, &phpng_retval, param_count, real_params, no_separation, NULL);
}


  int sw_zend_is_callable(zval *cb, int a, char **name)
 {
     zend_string *key = NULL;
     int ret = zend_is_callable(cb, a, &key);
     char *tmp = estrndup(key->val, key->len);
     zend_string_release(key);
     *name = tmp;
     return ret;
 }

//PHP_FUNCTION(tinys_set_on)
PHP_METHOD(tinys, on)
{
	zval *name;	//事件名称
	zval *cb;	//回调函数
	char *func_name = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS()TSRMLS_CC, "zz", &name, &cb) == FAILURE)
	{
		return;
	}
	if (!sw_zend_is_callable(cb, 0, &func_name TSRMLS_CC))
	{
		printf("Function '%s' is not callable", func_name);
		efree(func_name);
		return;
	}

	printf("Function '%s' ", func_name);
	efree(func_name);

	convert_to_string(name);

	char *callback_name[PHP_SERVER_CALLBACK_NUM] = {
	        "Connect",
	        "Receive",
	    };
	int i;
	char property_name[128];
	int l_property_name = 0;
	memcpy(property_name, "on", 2);


	for (i = 0; i < PHP_SERVER_CALLBACK_NUM; i++)
	{
		if (callback_name[i] == NULL)
		{
			continue;
		}
		if (strncasecmp(callback_name[i], Z_STRVAL_P(name), Z_STRLEN_P(name)) == 0)
		{

			printf("set callback '%s' i %d ", callback_name[i],i);
//			php_sw_server_callbacks[i] = cb;
			 memcpy(property_name + 2, callback_name[i], Z_STRLEN_P(name));
			l_property_name = Z_STRLEN_P(name) + 2;
			property_name[l_property_name] = '\0';
			zend_update_property(tinys_ce, getThis(), property_name, l_property_name, cb TSRMLS_CC);
			php_sw_server_callbacks[i] = sw_zend_read_property(tinys_ce, getThis(), property_name, l_property_name, 0 TSRMLS_CC);


			break;
		}
	}

}

/* {{{ proto int server()
   char* ip,int port); */
//PHP_FUNCTION(tinys_server)
PHP_METHOD(tinys, run)

{
	char * ip;
	size_t ip_len;
	long port_in;
	int port;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl",&ip,  &ip_len, &port_in) == FAILURE) {
	        RETURN_NULL();
	    }

	//调用c函数执行
	printf("to run server\n");
	port = (int)port_in;
	server(ip,port);

//	php_error(E_WARNING, "server: not yet implemented");
}
/* }}} */

PHP_METHOD(tinys, send)
{
	printf("tinys_send \n");

	zval *zobject = getThis();

	printf("tinys_send 1 \n");
	int ret;

	zval *zfd;
	zval *zdata;
	long server_socket = -1;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz|l", &zfd, &zdata, &server_socket) == FAILURE)
	{
		return;
	}

	printf("tinys_send 2 \n");
	char *data;
//	int length = php_swoole_get_send_data(zdata, &data TSRMLS_CC);
	convert_to_string(zdata);
	printf("tinys_send 2.1 \n");
	int length = Z_STRLEN_P(zdata);
	printf("tinys_send 2.2 length %l \n",length);
	data = Z_STRVAL_P(zdata);
//	data = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
//	response = data;
//	char response[200];

//	 strcpy(response, data);
//	 printf("res1 char %c \n",response[20]);
//	 printf("res1 char %c \n",response[1]);
//	 printf("res1 char %c \n",response[10]);


	printf("tinys_send 3 \n");

	printf("send data %s",data);

	if (length < 0)
	{
		RETURN_FALSE;
	}
	else if (length == 0)
	{
		printf("data is empty.");
		RETURN_FALSE;
	}
	printf("tinys_send setp 4 \n");
	convert_to_long(zfd);
	uint32_t fd = (uint32_t) Z_LVAL_P(zfd);
//
//
	//修改epoll状态,发送数据
//	data = "send back \n";
	setOutPut(data,fd,length);

//	ret = swSocket_write_blocking(fd,data,length);
	if (ret < 0)
	{
		printf("sendto to reactor failed. Error: %s [%d]", strerror(errno), errno);
	}
//	return ret;
	SW_CHECK_RETURN(ret);
}


int swSocket_write_blocking(int __fd, void *__data, int __len)
{

	printf("swSocket_write_blocking setp 1 \n");
    int n = 0;
    int written = 0;

    while (written < __len)
    {
    	printf("swSocket_write_blocking setp 2 __len %d __fd %d \n",__len,__fd);
		n = write(__fd, __data + written, __len - written);
		printf("swSocket_write_blocking setp 3 n %d \n",n);
		if (n < 0)
		{
			printf("errno %d \n",errno);
            if (errno == EINTR)
            {
                continue;
            }
            else if (errno == EAGAIN)
            {
                swSocket_wait(__fd, SW_WORKER_WAIT_TIMEOUT, SW_EVENT_WRITE);
                continue;
            }
            else
            {
                printf("write %d bytes failed.", __len);
                return SW_ERR;
            }
        }
        written += n;
    }

    return written;
}


/**
 * Wait socket can read or write.
 */
int swSocket_wait(int fd, int timeout_ms, int events)
{
    struct pollfd event;
    event.fd = fd;
    event.events = 0;

    if (events & SW_EVENT_READ)
    {
        event.events |= POLLIN;
    }
    if (events & SW_EVENT_WRITE)
    {
        event.events |= POLLOUT;
    }
    while (1)
    {
        int ret = poll(&event, 1, timeout_ms);
        if (ret == 0)
        {
            return SW_ERR;
        }
        else if (ret < 0 && errno != EINTR)
        {
        	printf("poll() failed. Error: %s[%d]", strerror(errno), errno);
            return SW_ERR;
        }
        else
        {
            return SW_OK;
        }
    }
    return SW_OK;
}




//然后，定义一个zend_function_entry
zend_function_entry tinys_method[]=
{
//    ZEND_ME(tinys,    __construct,    NULL,   ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
//    ZEND_ME(tinys,    ToArray,  NULL,   ZEND_ACC_PUBLIC)
//    ZEND_ME(tinys,    Instance,  NULL,   ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
    ZEND_ME(tinys,    on,  NULL,   ZEND_ACC_PUBLIC)
    ZEND_ME(tinys,    run,  NULL,   ZEND_ACC_PUBLIC)
    ZEND_ME(tinys,    send,  NULL,   ZEND_ACC_PUBLIC)
//    ZEND_ME(tinys,    GroupBy,  NULL,   ZEND_ACC_PUBLIC)
//    ZEND_ME(tinys,    Concat,  NULL,   ZEND_ACC_PUBLIC)
    // ZEND_ME(tinys,    GetApplicables,  NULL,   ZEND_ACC_PUBLIC)
    {NULL,  NULL,   NULL}
};


/* {{{ php_tinys_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_tinys_init_globals(zend_tinys_globals *tinys_globals)
{
	tinys_globals->global_value = 0;
	tinys_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(tinys)
{
	/* If you have INI entries, uncomment these lines
	REGISTER_INI_ENTRIES();
	*/
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce,"tinys",tinys_method);
	tinys_ce = zend_register_internal_class(&ce TSRMLS_CC);


	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(tinys)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(tinys)
{
#if defined(COMPILE_DL_TINYS) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(tinys)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(tinys)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "tinys support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */




/* {{{ tinys_functions[]
 *
 * Every user visible function must have an entry in tinys_functions[].
 */
const zend_function_entry tinys_functions[] = {
//	PHP_FE(tinys_server,	arginfo_tinys_server)
//	PHP_FE(tinys_set_on,	arginfo_tinys_set_on)
	PHP_FE_END	/* Must be the last line in tinys_functions[] */
};
/* }}} */





/* {{{ tinys_module_entry
 */
zend_module_entry tinys_module_entry = {
	STANDARD_MODULE_HEADER,
	"tinys",
	tinys_functions,
	PHP_MINIT(tinys),
	PHP_MSHUTDOWN(tinys),
	PHP_RINIT(tinys),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(tinys),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(tinys),
	PHP_TINYS_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_TINYS
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(tinys)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
