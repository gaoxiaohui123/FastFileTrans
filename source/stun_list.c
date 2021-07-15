
#include "file_rtp.h"

int stun_create_node(CStunNode **head0)
{
    CStunNode *head = *head0;
    if(!head)
    {
        //MYPRINT("PushData: create head: obj->data_list=%x \n", obj->data_list);
        head = (CStunNode *)calloc(1, sizeof(CStunNode));  //创建头节点。
        head->num = 0;
        head->id = 0;
        head->next = NULL;  //头节点指针域置NULL
        head->tail = head;  // 开始时尾指针指向头节点
        *head0 = head;
    }
    else{
        return -1;
    }
    return 0;
}

void stun_add_node(CStunNode *head, CStunNode **pnew)
{
    //MYPRINT2("stun_add_node: (*pnew)=%x \n", (*pnew));
    if(!(*pnew))
    {
        (*pnew) = calloc(1, sizeof(CStunNode));
    }
    //CStunNode *head = head0;//obj->broadCastHead;
    (*pnew)->next = NULL;   //新节点指针域置NULL
    head->tail->next = *pnew;  //新节点插入到表尾
    head->tail = *pnew;   //为指针指向当前的尾节点
    head->id++;
    head->num++;
    (*pnew)->id = head->id;
    //MYPRINT2("stun_add_node: head->num=%d, (*pnew)->id=%d \n", head->num, (*pnew)->id);

}
void stun_add_node_by_id(CStunNode *head, char *data, int size, int id)
{
    void *ret = 0;
    CStunNode *p;
    if(!head)
    {
        return;
    }
    p = head;
    do{
        p = p->next;
        CStunNode *info = (CStunNode *)p;
        if(p && (p->id < id))
        {
            ret = (void *)p;
            break;
        }
    }while(p->next);
}
void *stun_find_node_by_id(CStunNode *head, int id)
{
    void *ret = 0;
    CStunNode *p;
    if(!head)
    {
        return ret;
    }
    p = head;
    do{
        p = p->next;
        CStunNode *info = (CStunNode *)p;
        if(p && (p->id == id))
        {
            ret = (void *)p;
            break;
        }
    }while(p->next);
    return ret;
}
void *stun_find_node(CStunNode *head)
{
    void *ret = 0;
    CStunNode *p;
    if(!head)
    {
        return ret;
    }
    p = head;
    do{
        p = p->next;
        CStunNode *info = (CStunNode *)p;
        if(p)
        {
            ret = (void *)p;
            break;
        }
    }while(p->next);
    return ret;
}

void stun_delete_node(CStunNode *head)
{
    CStunNode *ret = NULL;
    CStunNode *p,*q;
    if(!head)
    {
        return;
    }
    MYPRINT2("stun_delete_node: 0: head->num=%d \n", head->num);
    q = head;
    p = head->next;
    while(p && (p != head))
    {
        MYPRINT2("stun_delete_node: p->id=%d \n", p->id);
        if(p != head)
        {
            ret = p;
            break;
        }
        q = p;//last
        p = p->next;
    }
    if(ret)
    {
        //MYPRINT2("deletenode: ret->addr_client.sin_port=%d, ret->addr_client.sin_addr.s_addr=%lld \n", ret->addr_client.sin_port, ret->addr_client.sin_addr.s_addr);
        q->next = p->next;
        if(head->next == NULL)
        {
            //有效节点全部被删除
            MYPRINT2("stun_delete_node: head->next=%x \n", head->next);
            head->next = NULL;  //头节点指针域置NULL
            head->tail = head;
        }
        else if(q->next == NULL)
        {
            //尾部节点被删除//凡未遵循先入先出原则的都需要进行尾部节点检测!!!
            MYPRINT2("stun_delete_node: q->next=%x \n", q->next);
            head->tail->next = q;
            head->tail = q;
        }
        if(ret->data)
        {
            free(ret->data);
        }
        free(ret);
        head->num--;
        MYPRINT2("stun_delete_node: head->num=%d \n", head->num);

        return;
    }
    else{
        MYPRINT2("stun_delete_node: cannot find: head->num=%d \n", head->num);

    }
    return;
}

void stun_free_node(CStunNode *head)
{
    CStunNode *p,*q;
    if(!head)
    {
        return;
    }
    p = head;
    while (p->next != NULL) {
        q = p->next;
        p->next = q->next;
        if(q->data)
        {
            free(q->data);
        }
        free(q);
    }
    //free(p);
    free(head);   //最后删除头节点
    printf("stun_free_node: ok \n");
}
