#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>


#include "./include/error.h"
#include "./include/parse_arg.h"
#include "./include/stream.h"
#include "./include/thread_pool.h"

void* foo(void* arg) {
	sleep(1);
	printf("hello world\n");
	return NULL;
}

void* boo(void* arg) {
	int* a = (int*)arg;
	int b = *a;
	int* c = (int*)calloc(1, sizeof(int));
	*c = b * 2;
	return c;
}

int main(int argc, char* argv[]) {
	const uint32_t tread_num = get_thread_num(argc, argv);
	if(tread_num == 0) {
		return 1;
	}
	thread_pool* th = thread_pool_init(tread_num);
	if(th == NULL) {
		return 1;
	}
	deffer_free_vector_v2(vc_v2) = in_stream_v2(fileno(stdin));
	if(vc_v2 == NULL) {
		thread_pool_destroy(th);
		return 1;
	}
	// thread_pool_activate(th);
	// thread_pool_push_task(th, foo, NULL);
	// thread_pool_push_task(th, foo, NULL);
	// thread_pool_push_task(th, foo, NULL);
	// thread_pool_push_task(th, foo, NULL);
	// thread_pool_push_task(th, foo, NULL);


	// int arr[4] = {1, 2, 3, 4};
	// task_handle* handle1 = thread_pool_push_task_joinable(th, boo, &arr[0]);
	// task_handle* handle2 = thread_pool_push_task_joinable(th, boo, &arr[1]);
	// task_handle* handle3 = thread_pool_push_task_joinable(th, boo, &arr[2]);
	// task_handle* handle4 = thread_pool_push_task_joinable(th, boo, &arr[3]);
	
	// int res[4] = {0};
	// int* a = (int*)thread_pool_task_wait(handle1);
	// int* b = (int*)thread_pool_task_wait(handle2);
	// int* c = (int*)thread_pool_task_wait(handle3);
	// int* d = (int*)thread_pool_task_wait(handle4);

	// res[0] = *a;
	// res[1] = *b;
	// res[2] = *c;
	// res[3] = *d;
	
	// for(int i = 0; i < 4; i++) {
		// 	printf("%d\n", res[i]);
		// }
		
		
	vector_sort_v2(vc_v2, th, tread_num);
	thread_pool_deactivate(th);
	out_stream_v2(vc_v2, fileno(stdout));
	thread_pool_destroy(th);
	return 0;
}


