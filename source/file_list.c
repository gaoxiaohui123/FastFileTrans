
#include "file_rtp.h"



int file_create_node(FileNode **head)
{
    //FileNode *head = *head0;
    if(!head)
    {
        //MYPRINT("PushData: create head: obj->data_list=%x \n", obj->data_list);
        head = (FileNode *)calloc(1, sizeof(FileNode));  //创建头节点。
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

void file_add_node(FileNode *head, FileNode **pnew)
{
    //MYPRINT2("file_add_node: (*pnew)=%x \n", (*pnew));
    if(!(*pnew))
    {
        (*pnew) = calloc(1, sizeof(FileNode));
    }
    //FileNode *head = head0;//obj->broadCastHead;
    (*pnew)->next = NULL;   //新节点指针域置NULL
    head->tail->next = *pnew;  //新节点插入到表尾
    head->tail = *pnew;   //为指针指向当前的尾节点
    head->id++;
    head->num++;
    (*pnew)->id = head->id;
    //MYPRINT2("file_add_node: head->num=%d, (*pnew)->id=%d \n", head->num, (*pnew)->id);

}
void *file_find_node(FileNode *head)
{
    void *ret = 0;
    FileNode *p;
    if(!head)
    {
        return ret;
    }
    p = head;
    do{
        p = p->next;
        FileNode *info = (FileNode *)p;
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

void file_delete_node(FileNode *head)
{
    FileNode *ret = NULL;
    FileNode *p,*q;
    if(!head)
    {
        return;
    }
    MYPRINT2("file_delete_node: 0: head->num=%d \n", head->num);
    q = head;
    p = head->next;
    while(p && (p != head))
    {
        MYPRINT2("file_delete_node: p->id=%d \n", p->id);
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
                    MYPRINT2("error: file_delete_node: all zero \n");
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
            MYPRINT2("file_delete_node: head->next=%x \n", head->next);
            head->next = NULL;  //头节点指针域置NULL
            head->tail = head;
        }
        else if(q->next == NULL)
        {
            //尾部节点被删除//凡未遵循先入先出原则的都需要进行尾部节点检测!!!
            MYPRINT2("file_delete_node: q->next=%x \n", q->next);
            head->tail->next = q;
            head->tail = q;
        }
        if(ret->data)
        {
            free(ret->data);
        }
        free(ret);
        head->num--;
        MYPRINT2("file_delete_node: head->num=%d \n", head->num);

        return;
    }
    else{
        MYPRINT2("file_delete_node: cannot find: head->num=%d \n", head->num);

    }
    return;
}

void file_free_node(FileNode *head)
{
    FileNode *p,*q;
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
}
