
#include "file_rtp.h"

extern int frame_create_node(FrameNode **head0);
extern void frame_add_node(FrameNode *head0, FrameNode **pnew);
extern void *frame_find_node(FrameNode *head);
extern void frame_delete_node(FrameNode *head);
extern void frame_free_node(FrameNode *head);

int pic_create_node(PicNode **head0)
{
    PicNode *head = *head0;
    if(!head)
    {
        //MYPRINT("PushData: create head: obj->data_list=%x \n", obj->data_list);
        head = (PicNode *)calloc(1, sizeof(PicNode));  //创建头节点。
        head->num = 0;
        head->id = 0;
        //frame_create_node(&head->head);
        head->next = NULL;  //头节点指针域置NULL
        head->tail = head;  // 开始时尾指针指向头节点
        *head0 = head;
    }
    else{
        return -1;
    }
    return 0;
}

void pic_add_node(PicNode *head, PicNode **pnew)
{
    if(!(*pnew))
    {
        (*pnew) = calloc(1, sizeof(PicNode));
        frame_create_node(&((*pnew)->head));
    }
    //PicNode *head = head0;//obj->broadCastHead;
    if(head->num > 100)
    {
        MYPRINT2("pic_add_node: head->num=%d \n", head->num);
        pic_delete_node(head);
    }
    (*pnew)->next = NULL;   //新节点指针域置NULL
    head->tail->next = *pnew;  //新节点插入到表尾
    head->tail = *pnew;   //为指针指向当前的尾节点
    head->id++;
    head->num++;
    (*pnew)->id = head->id;
    //MYPRINT2("pic_add_node: head->num=%d, (*pnew)->id=%d \n", head->num, (*pnew)->id);

}
#if 1
void *pic_find_node_by_id(PicNode *head, int id)
{
    void *ret = 0;
    PicNode *p;
    if(!head)
    {
        return ret;
    }
    p = head;
    do{
        p = p->next;
        PicNode *info = (PicNode *)p;
        if(p && (p->id == id))
        {
            ret = (void *)p;
            break;
        }
    }while(p->next);
    return ret;
}void pic_delete_node_by_id(PicNode *head, int id)
{
    PicNode *ret = NULL;
    PicNode *p,*q;
    if(!head)
    {
        return;
    }
    MYPRINT2("pic_delete_node: 0: head->num=%d \n", head->num);
    q = head;
    p = head->next;
    while(p && (p != head))
    {
        MYPRINT2("pic_delete_node: p->id=%d \n", p->id);
        if((p->id == id) && p != head)
        {
            ret = p;
            break;
        }
        q = p;//last
        p = p->next;
    }
    if(ret)
    {
        q->next = p->next;
        if(head->next == NULL)
        {
            //有效节点全部被删除
            MYPRINT2("pic_delete_node: head->next=%x \n", head->next);
            head->next = NULL;  //头节点指针域置NULL
            head->tail = head;
        }
        else if(q->next == NULL)
        {
            //尾部节点被删除//凡未遵循先入先出原则的都需要进行尾部节点检测!!!
            MYPRINT2("pic_delete_node: q->next=%x \n", q->next);
            head->tail->next = q;
            head->tail = q;
        }
        frame_free_node(ret->head);
        free(ret);
        head->num--;
        MYPRINT2("pic_delete_node: head->num=%d \n", head->num);

        return;
    }
    else{
        MYPRINT2("pic_delete_node: cannot find: head->num=%d \n", head->num);

    }
    return;
}
#else
void *pic_find_node(PicNode *head)
{
    void *ret = 0;
    PicNode *p;
    if(!head)
    {
        return ret;
    }
    p = head;
    do{
        p = p->next;
        PicNode *info = (PicNode *)p;
        if(p)
        {
#if 0
            struct sockaddr_in *paddr = (struct sockaddr_in *)&info->addr_client;
            int flag = (paddr->sin_port - addr_client.sin_port) | (paddr->sin_addr.s_addr - addr_client.sin_addr.s_addr);
            if(!flag)
            {
                ret = (void *)p;
                break;
            }
#endif
        }
    }while(p->next);
    return ret;
}
#endif
void pic_delete_node(PicNode *head)
{
    PicNode *ret = NULL;
    PicNode *p,*q;
    if(!head)
    {
        return;
    }
    MYPRINT2("pic_delete_node: 0: head->num=%d \n", head->num);
    q = head;
    p = head->next;
    while(p && (p != head))
    {
        MYPRINT2("pic_delete_node: p->id=%d \n", p->id);
        //struct sockaddr_in *paddr = (struct sockaddr_in *)&p->addr_client;
        int flag = 0;//(p->addr_client.sin_port - addr_client.sin_port) | (p->addr_client.sin_addr.s_addr - addr_client.sin_addr.s_addr);
        if(!flag && p != head)
        {
            ret = p;
            break;
        }
        else{
            //if(p->addr_client.sin_port | p->addr_client.sin_addr.s_addr | addr_client.sin_port | addr_client.sin_addr.s_addr)
            if(flag)
            {
                
            }
            else{
                if(p->next != head)
                {
                    MYPRINT2("error: pic_delete_node: all zero \n");
                    break;
                }
            }
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
            MYPRINT2("pic_delete_node: head->next=%x \n", head->next);
            head->next = NULL;  //头节点指针域置NULL
            head->tail = head;
        }
        else if(q->next == NULL)
        {
            //尾部节点被删除//凡未遵循先入先出原则的都需要进行尾部节点检测!!!
            MYPRINT2("pic_delete_node: q->next=%x \n", q->next);
            head->tail->next = q;
            head->tail = q;
        }
        frame_delete_node(ret->head);
        free(ret);
        head->num--;
        MYPRINT2("pic_delete_node: head->num=%d \n", head->num);

        return;
    }
    else{
        MYPRINT2("pic_delete_node: cannot find: head->num=%d \n", head->num);

    }
    return;
}

void pic_free_node(PicNode *head) {
    PicNode *p,*q;
    if(!head)
    {
        return;
    }
    p = head;
    while (p->next != NULL) {
        q = p->next;
        p->next = q->next;
        frame_free_node(q->head);
        free(q);
    }
    //free(p);
    free(head);   //最后删除头节点
}
