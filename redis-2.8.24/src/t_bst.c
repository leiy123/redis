#include "redis.h"

typedef void (*Visit)(redisClient *c, robj *obj);
typedef void (*Convert)(bstNode *c, list *list);


bstNode *bstCreateNode(robj *obj) {
    bstNode *bn = zmalloc(sizeof(bstNode));
    bn->obj = obj;
    return bn;
}

bst *bstCreate(void) {
    bst *bt;
    bt = zmalloc(sizeof(*bt));
	bt->root = NULL;
	bt->num = 0;
    return bt;
}

void bstFreeNode(bstNode *node) {
    decrRefCount(node->obj);
    zfree(node);
}

void _bstFree(bstNode *root) {
    bstNode *node = root;

    if(node->lchild) _bstFree(node->lchild);
	if(node->rchild) _bstFree(node->rchild);
	zfree(root);
}

void bstFree(bst *bt){
	_bstFree(bt->root);
	zfree(bt);
}
//location: no dulplication no recursive
bstNode *bstSearch(bst *bt,  robj *obj){
	printf("=========before bstSearch=========\n");
	redisAssert(bt != NULL);
	printf("=========bstSearch=========\n");
	bstNode *cur = bt->root, *next = bt->root;
	//empty tree root:NULL
	if(!bt->root) return bt->root;
	while(next != NULL){
			while(next != NULL && compareStringObjects(next->obj, obj) < 0){
				cur = next;
				next = next->rchild;
				}
			if(next == NULL) return cur; //cur->rchild
			if(compareStringObjects(next->obj, obj)==0) return next;
			if(compareStringObjects(next->obj, obj)>0){
				cur = next;
			}
		next = cur->lchild;
	}
	if(next == NULL) return cur; //cur->lchild
}

int bstInsert(bst *bt, robj *obj) {
	bstNode *node = bstSearch(bt, obj);
	//empty tree
	printf("===========after bstsearch=================\n");
	if(node == NULL) {
		bt->root = bstCreateNode(obj);
		incrRefCount(obj); //createoject:1, ops:++
		bt->num++;
		return 1;
	}
	
	//otherwise
	int res = compareStringObjects(node->obj, obj);
	if(res == 0) return 0;
	else{
		bstNode *new = bstCreateNode(obj);
		incrRefCount(obj);
		if(res < 0) {
			node->rchild = new;
		}else{
			node->lchild = new;
		}
		bt->num++;
		return 1;
	}
}
//helper:del root;
bstNode *findLMax(bstNode *root, bstNode **pre, int *ret){
	redisAssert(root != NULL);
	bstNode *cur = root->lchild;
	*pre = root;
	if(cur == NULL){
		if(root->rchild != NULL){
			*ret = 1;
			return root->rchild;
		} else { //L/Rchild
			*ret = 2;
			return NULL;
		}
	} 
	
	while(cur->rchild != NULL){
		*pre = cur; //odd: when return back pre turns NULL
		cur = cur->rchild;
	}
	if(cur->lchild != NULL) *ret = 3;
		else *ret = 4;
	printf("=======ret: %d=======\n",*ret);
	return cur;
}

//helper for bstDel in case 2; prerequirement: locate node in bst; only for leaf node
bstNode *pNode(bst *bt, bstNode *node, int *lr){
	bstNode *pnode = bt->root, *cur = bt->root;
	int ret;	
	while(1){
		ret = compareStringObjects(node->obj, cur->obj);
		if(ret > 0){
			pnode = cur;
			cur = pnode->rchild;
			*lr = 1; //rchild
		} else if(ret < 0){
			pnode = cur;
			cur = pnode->lchild;
			*lr = -1;
		} else{
			return pnode;
		}			
	}
}


int bstDel(bst *bt, robj *obj){
	bstNode *rep, *pre, *node, *pnode;
	int ret, lr;
	robj *rj;
	
	node = bstSearch(bt, obj);
	if(node == NULL || (compareStringObjects(node->obj, obj) != 0) )
		return 0;
	
	if(node == bt->root && !node->lchild && !node->rchild){
		bt->num--;
		bstFreeNode(bt->root);
		//bt->root = NULL;
		return 1;
	}
	//at lease two nodes
	else {
			rj = node->obj;
			rep = findLMax(node, &pre, &ret);
			printf("=======in bstdel ret: %d=============\n", ret);
			if(!pre){
				printf("=======rep nullllllllllllllllll========\n");
				return 3;
			}
			//lr > 0?pnode->rchild:pnode->lchild = NULL;	
			switch(ret){ 
				case 1: node->obj = rep->obj; node->rchild = rep->rchild; node->lchild =rep->lchild; break;
				case 2: rep = node; pnode = pNode(bt, node, &lr); break;
				case 3: pre->rchild = rep->lchild; node->obj = rep->obj; printf("====case 3===\n"); break;
				case 4:  node->obj = rep->obj; node->lchild = NULL; break;
				default:  return 5;
			}
		  bt->num--;
		  if(ret == 2){
			  if(lr > 0)
				  pnode->rchild = NULL;
			  else
				  pnode->lchild = NULL;
			  
		  } else{
			  //incrRefCount(rep->obj);
			  rep->obj = rj;
		  }
		  bstFreeNode(rep); //free failure
		  return 1;
	}
 }

//oLR: for underline storage
void addTolist(bstNode *c, list *list){
	list = listAddNodeTail(list, c->obj);
}
int _bstTolist(bstNode *root, Convert convert, list *li){
		if(root){
			convert(root, li);
			if(_bstTolist(root->lchild, convert, li)){
				if(_bstTolist(root->rchild, convert, li)){
					return 1;
				}
			} else 
				return 0;
		} else {
			return 1;
	}
}

list *bstTolist(bstNode *root, list *li){
	_bstTolist(root, addTolist, li);
	return li;
}




//command: bstadd key value
void ws_BSTinsertCommand(redisClient *c){
	int j, added = 0, status = 0;
	robj *ele;
	robj *key = c->argv[1];
	robj *bstobj = lookupKeyWrite(c->db,key);
	if(bstobj == NULL){
		bstobj = createBstObject();
		dbAdd(c->db,key,bstobj);
		printf("1111111\n");
	} else{
		if(bstobj->type != REDIS_BST) {
            addReply(c,shared.wrongtypeerr);
		}
	}
	
	
   if(bstobj->encoding == REDIS_ENCODING_BST){
	   for(j = 2; j < c->argc; j++){
			ele = c->argv[j] = tryObjectEncoding(c->argv[j]);
			if(bstInsert((bst *)bstobj->ptr, ele) != 0){
				incrRefCount(ele);
				server.dirty++;
				added++;
				status = 1;
			};
		}
   }
	else{
		 redisPanic("Unknown bst encoding");
	}
	//reply to client
	addReplyLongLong(c, status?added: 0);
}

//
void ws_BSTdelCommand(redisClient *c){
	 bst *bt; 
	 robj *bstobj, *key;
	 int j, ret;
	 int deleted = 0, keyremoved = 0;
	 key = c->argv[1];
	 if ((bstobj = lookupKeyReadOrReply(c,key,shared.emptymultibulk)) == NULL
         || checkType(c,bstobj,REDIS_BST)) return;
		 
	if (bstobj->encoding != REDIS_ENCODING_BST)
		redisPanic("Unknown bst encoding");
		
	 bt = (bst *)bstobj->ptr;
	 
	       for (j = 2; j < c->argc; j++) {  //argv[j]: REDIS_STRING
				ret = bstDel(bt, c->argv[j]);
				if (ret == 1) {
					deleted++;
					
					if (bt->num == 0) {
						dbDelete(c->db,key);
						keyremoved = 1;
						break;
					}
				} else if(ret == 0){
					
				} else{
					redisPanic("=======missing some case222========");
				}
        }
		
	 if (deleted) {
        notifyKeyspaceEvent(REDIS_NOTIFY_ZSET,"bstdel",key,c->db->id);
        if (keyremoved)
            notifyKeyspaceEvent(REDIS_NOTIFY_GENERIC,"del",key,c->db->id);
        signalModifiedKey(c->db,key);
        server.dirty += deleted;
    }
    addReplyLongLong(c,deleted);
}

void ws_BSTupdateCommand(redisClient *c){
	
}

//ws_BSTsearch bst meanlingless -----> traverseCommand
void ws_BSTsearchCommand(redisClient *c){
	bst *bt; 
	 robj *key = c->argv[1];
	 //int bstlen;
	 robj *bstobj;
	 if ((bstobj = lookupKeyReadOrReply(c,key,shared.emptymultibulk)) == NULL
         || checkType(c,bstobj,REDIS_BST)) return;
		
	 bt = (bst *)bstobj->ptr;
	 //traverse whole list
	 /*
	 bstlen = bt->num;
	 addReplyMultiBulkLen(c, bstlen);
	 node = bt->root;
	 while(bstlen--){
	 */ 
	 robj *ele = c->argv[2] = tryObjectEncoding(c->argv[2]);
	 if((compareStringObjects(bstSearch(bt, ele)->obj, ele)) == 0){
		 addReplyStatus(c, "in bst");
	 } else{
		  addReplyStatus(c, "not in bst");	
		}
}
//ws_BSTtraverse key: LOR
int ws_bstIntr(bstNode *root, Visit visit, redisClient *c){
	if(root){
		printf("=======traverse 247=========\n");
		if(ws_bstIntr(root->lchild, visit, c)){
			printf("=======lchild 248=============\n");
			visit(c, root->obj);
			printf("=======root 251=============\n");
			if(ws_bstIntr(root->rchild, visit, c)){
				printf("=======rchild 252=============\n");
				return 1;
				}
		} else 
			return 0;
	} else {
		printf("==========traverse 255==============\n");
		return 1;
	}	 
}


void ws_BSTtraverseCommand(redisClient *c){
	 long len;
	 robj *bstobj;
	 bst *bt;
	 
	 if ((bstobj = lookupKeyReadOrReply(c,c->argv[1],shared.emptymultibulk)) == NULL
         || checkType(c,bstobj,REDIS_BST)) return;
	
	 bt = (bst *)bstobj->ptr;	
	 len = bt->num;
	 printf("=========len: %l============\n", len);
	 
	 addReplyMultiBulkLen(c,len);
	 ws_bstIntr(bt->root, addReplyBulk, c);
}
