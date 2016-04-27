#include "redis.h"


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
	printf("=========before bstSearch=========");
	redisAssert(bt != NULL);
	printf("=========bstSearch=========");
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
	printf("===========after bstsearch=================");
	if(node == NULL) {
		bt->root = bstCreateNode(obj);
		bt->num++;
		return 1;
	}
	
	//otherwise
	int res = compareStringObjects(node->obj, obj);
	if(res == 0) return 0;
	else{
		bstNode *new = bstCreateNode(obj);
		if(res < 0) {
			node->rchild = new;
		}else{
			node->lchild = new;
		}
		bt->num++;
		return 1;
	}
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
		printf("1111111");
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

void ws_BSTdelCommand(redisClient *c){
	
}
void ws_BSTupdateCommand(redisClient *c){
	
}

//ws_BSTsearch bst meanlingless -----> traverseCommand
void ws_BSTsearchCommand(redisClient *c){
	bst *bt; 
	bstNode *node;
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

