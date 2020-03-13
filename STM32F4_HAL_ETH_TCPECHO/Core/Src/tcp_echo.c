/*
 * tcp_echo.c
 *
 *  Created on: 2020. 3. 12.
 *      Author: KIKI
 */

#include "tcp_echo.h"

/* 서버용 PCB 포인터*/
static struct tcp_pcb *tcp_echoserver_pcb;

/* 콜백함수 */
static err_t tcp_echoserver_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
static err_t tcp_echoserver_recv(void *arg, struct tcp_pcb *tpcb,
		struct pbuf *p, err_t err);
static void tcp_echoserver_error(void *arg, err_t err);
static err_t tcp_echoserver_poll(void *arg, struct tcp_pcb *tpcb);
static err_t tcp_echoserver_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);

/* 데이터 전송 */
static void tcp_echoserver_send(struct tcp_pcb *tpcb,
		struct tcp_echoserver_struct *es);

/* 연결 종료 */
static void tcp_echoserver_connection_close(struct tcp_pcb *tpcb,
		struct tcp_echoserver_struct *es);

/* 초기화 */
void tcp_echoserver_init(void)
{
	/* PCB 생성 */
	tcp_echoserver_pcb = tcp_new();

	if (tcp_echoserver_pcb != NULL)
	{
		err_t err;

		/* 7번 포트 바인딩 */
		err = tcp_bind(tcp_echoserver_pcb, IP_ADDR_ANY, 7);

		if (err == ERR_OK)
		{
			/* 소켓 리스닝 */
			tcp_echoserver_pcb = tcp_listen(tcp_echoserver_pcb);

			/* accept 콜백 지정 */
			tcp_accept(tcp_echoserver_pcb, tcp_echoserver_accept);
		}
		else
		{
			/* 초기화 오류 시 메모리 해제 */
			memp_free(MEMP_TCP_PCB, tcp_echoserver_pcb);
		}
	}
}

/* Accept 콜백함수 */
static err_t tcp_echoserver_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
	err_t ret_err;
	struct tcp_echoserver_struct *es;

	LWIP_UNUSED_ARG(arg);
	LWIP_UNUSED_ARG(err);

	/* 새로운 연결에 우선순위 지정 */
	tcp_setprio(newpcb, TCP_PRIO_MIN);

	/* 상태 관리용 구조체 변수 할당*/
	es = (struct tcp_echoserver_struct*) mem_malloc(
			sizeof(struct tcp_echoserver_struct));
	if (es != NULL)
	{
		es->state = ES_ACCEPTED; //Accept 완료 상태
		es->pcb = newpcb;        //Accept 시 할당 받은 소켓용 PCB
		es->retries = 0;
		es->p = NULL;            //송수신 버퍼

		/* 이 연결에 Callback 이 호출될 경우 es 를 인자로 전달 */
		tcp_arg(newpcb, es);

		/* Recv Callback 지정 */
		tcp_recv(newpcb, tcp_echoserver_recv);

		/* Error Callback 지정  */
		tcp_err(newpcb, tcp_echoserver_error);

		/* Poll Callback 지정 */
		tcp_poll(newpcb, tcp_echoserver_poll, 0);

		ret_err = ERR_OK;
	}
	else
	{
		/*  연결 관리용 구조체 할당 실패, 연결 종료  */
		tcp_echoserver_connection_close(newpcb, es);

		ret_err = ERR_MEM;
	}
	return ret_err;
}

/* 데이터 수신 Callback */
static err_t tcp_echoserver_recv(void *arg, struct tcp_pcb *tpcb,
		struct pbuf *p, err_t err)
{
	struct tcp_echoserver_struct *es;
	err_t ret_err;

	LWIP_ASSERT("arg != NULL", arg != NULL);

	es = (struct tcp_echoserver_struct*) arg;

	/* 수신 데이터가 없는 경우에는 연결 종료 처리, 소켓 read 함수가 0을 반환하는 케이스와 유사 */
	if (p == NULL)
	{
		/* 연결 종료 처리 */
		es->state = ES_CLOSING;
		if (es->p == NULL)
		{
			/* 더이상 처리할 데이터가 없고 바로 연결 종료 */
			tcp_echoserver_connection_close(tpcb, es);
		}
		else
		{
			/* 버퍼에 데이터가 존재하는 경우 */
			/* 송신 Callback 지정 */
			tcp_sent(tpcb, tcp_echoserver_sent);

			/* 잔여 데이터 전송*/
			tcp_echoserver_send(tpcb, es);
		}
		ret_err = ERR_OK;
	}
	else if (err != ERR_OK) //에러가 발생한 경우
	{
		/* 버퍼 정리 */
		if (p != NULL)
		{
			es->p = NULL;
			pbuf_free(p);
		}
		ret_err = err;
	}
	else if (es->state == ES_ACCEPTED) //최초 데이터 수신
	{
		/* 상태 변경 */
		es->state = ES_RECEIVED;

		/* 수신된 데이터 버퍼를 관리용 구조체에 저장 후 사용 */
		es->p = p;

		/* Send callback 지정 */
		tcp_sent(tpcb, tcp_echoserver_sent);

		/* 수신한 데이터를 그대로 전송, es->p에는 앞서 지정한 수신 Buffer 포인터가 들어가 있음 */
		tcp_echoserver_send(tpcb, es);

		ret_err = ERR_OK;
	}
	else if (es->state == ES_RECEIVED)
	{
		/* 데이터가 추가 수신된 경우 */
		if (es->p == NULL) //앞서 전송이 모두 완료되고 버퍼가 비어있는 경우
		{
			/* 추가 수신한 버퍼를 구조체에 저장해 둔다. */
			es->p = p;

			/* 데이터 전송 */
			tcp_echoserver_send(tpcb, es);
		}
		else //전송할 데이터가 아직 남아 있는 경우
		{
			struct pbuf *ptr;

			/* 추가 수신한 데이터 버퍼를 es->p 버퍼에 추가 한다. */
			ptr = es->p;
			pbuf_chain(ptr, p);
		}
		ret_err = ERR_OK;
	}
	else if (es->state == ES_CLOSING)
	{
		/* 연결 종료 수에 수신이 발생한 경우, 버퍼를 정리한다. */
		tcp_recved(tpcb, p->tot_len);
		es->p = NULL; //버퍼를 정리한다.
		pbuf_free(p);
		ret_err = ERR_OK;
	}
	else
	{
		/* 미정의 상태, 버퍼를 정리한다. */
		tcp_recved(tpcb, p->tot_len);
		es->p = NULL;
		pbuf_free(p);
		ret_err = ERR_OK;
	}
	return ret_err;
}

/* 에러 Callback */
static void tcp_echoserver_error(void *arg, err_t err)
{
	struct tcp_echoserver_struct *es;

	LWIP_UNUSED_ARG(err);

	es = (struct tcp_echoserver_struct*) arg;
	if (es != NULL)
	{
		/*  현재 연결에 문제가 발생한 경우 사용하던 구조체 변수를 해제한다. */
		mem_free(es);
	}
}

/* Poll Callback 주기적으로 호출된다. */
static err_t tcp_echoserver_poll(void *arg, struct tcp_pcb *tpcb)
{
	err_t ret_err;
	struct tcp_echoserver_struct *es;

	es = (struct tcp_echoserver_struct*) arg;
	if (es != NULL)
	{
		if (es->p != NULL) //버퍼에 데이터가 있는 경우
		{
			tcp_sent(tpcb, tcp_echoserver_sent); //Send callback을 지정한다.
			/* 버퍼에 남아있는 데이터를 전송한다. */
			tcp_echoserver_send(tpcb, es);
		}
		else
		{
			/* 버퍼가 비어 있는 경우  */
			if (es->state == ES_CLOSING)
			{
				/*  현재 종료해야 하는 연결은 종료 처리한다. */
				tcp_echoserver_connection_close(tpcb, es);
			}
		}
		ret_err = ERR_OK;
	}
	else
	{
		/* 구조체가 없는 연결은 해제한다. */
		tcp_abort(tpcb);
		ret_err = ERR_ABRT;
	}
	return ret_err;
}

/* Send Callback */
static err_t tcp_echoserver_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
	struct tcp_echoserver_struct *es;

	LWIP_UNUSED_ARG(len);

	es = (struct tcp_echoserver_struct*) arg;
	es->retries = 0;

	if (es->p != NULL) //버퍼에 데이터가 있는 경우
	{
		tcp_sent(tpcb, tcp_echoserver_sent); //Callback 지정
		tcp_echoserver_send(tpcb, es); //데이터 전송
	}
	else
	{
		/* 연결을 종료해야하고 있는 경우 Close 처리한다. */
		if (es->state == ES_CLOSING)
			tcp_echoserver_connection_close(tpcb, es);
	}
	return ERR_OK;
}

/* 데이터 전송 함수 */
static void tcp_echoserver_send(struct tcp_pcb *tpcb,
		struct tcp_echoserver_struct *es)
{
	struct pbuf *ptr;
	err_t wr_err = ERR_OK;

	/* 전송이 완료되지 안은 경우 */
	while ((wr_err == ERR_OK) && (es->p != NULL)
			&& (es->p->len <= tcp_sndbuf(tpcb)))
	{

		/* 전송할 버퍼 확인 */
		ptr = es->p;

		/* 데이터 전송 요청 */
		wr_err = tcp_write(tpcb, ptr->payload, ptr->len, 1);

		if (wr_err == ERR_OK)
		{
			u16_t plen;
			u8_t freed;

			plen = ptr->len;

			/* 버퍼가 Chaining 되어 있는 경우 */
			es->p = ptr->next;

			if (es->p != NULL)
			{
				/* 레퍼런스 카운터 증가 */
				pbuf_ref(es->p);
			}

			/* 전송한 버퍼 해제 */
			do
			{
				/* 버퍼 해제 */
				freed = pbuf_free(ptr);
			} while (freed == 0);
			/* 데이터 수신 윈도우 크기 지정 */
			tcp_recved(tpcb, plen);
		}
		else if (wr_err == ERR_MEM)
		{
			/* 전송 시 메모리 부족 버퍼 복원 */
			es->p = ptr;
		}
		else
		{
			/* other problem ?? */
		}
	}
}

/* 연결 종료 */
static void tcp_echoserver_connection_close(struct tcp_pcb *tpcb,
		struct tcp_echoserver_struct *es)
{

	/* Callback 해제 */
	tcp_arg(tpcb, NULL);
	tcp_sent(tpcb, NULL);
	tcp_recv(tpcb, NULL);
	tcp_err(tpcb, NULL);
	tcp_poll(tpcb, NULL, 0);

	/* 연결 관리용 구조체 삭제 */
	if (es != NULL)
	{
		mem_free(es);
	}

	/* 연결 종료 */
	tcp_close(tpcb);
}
