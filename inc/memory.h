#ifndef		_UCOMMON_MEMORY_H_
#define		_UCOMMON_MEMORY_H_

#ifndef		_UCOMMON_LINKED_H_
#include 	<ucommon/linked.h>
#endif

NAMESPACE_UCOMMON

class __EXPORT PagerPool;

class __EXPORT mempager
{
private:
	size_t	pagesize, align;
	unsigned count;

#pragma pack(1)
	typedef struct mempage {
		struct mempage *next;
		unsigned short used;		
	}	page_t;
#pragma pack()

	page_t *page;

protected:
	unsigned limit;

	inline size_t getOverhead(void)
		{return sizeof(page_t);};

	page_t *pager(void);
	
public:
	mempager(size_t ps = 0);
	virtual ~mempager();

	inline unsigned overhead(void)
		{return sizeof(page_t);};

	inline unsigned getPages(void)
		{return count;};

	inline unsigned getLimit(void)
		{return limit;};

	inline unsigned getAlloc(void)
		{return pagesize;};

	void release(void);
	virtual void dealloc(void *mem);
	virtual void *alloc(size_t size);
	char *dup(const char *cp);
	void *dup(void *mem, size_t size);

};

class __EXPORT PagerObject : public LinkedObject, public CountedObject
{
protected:
	friend class __EXPORT PagerPool;

	PagerPool *pager;

	PagerObject();

	void dealloc(void);
};	

class __EXPORT PagerPool : private mempager
{
private:
	size_t objsize;
	LinkedObject *freelist;
#if UCOMMON_THREADING > 0
	pthread_mutex_t mutex;
#endif

protected:
	PagerPool(size_t count, size_t objsize);
	PagerObject *get(void);

public:
	void put(PagerObject *obj);
};

class __EXPORT keyassoc : public mempager
{
private:
	class __EXPORT keydata : public NamedObject
	{
	public:
		void *data;

		keydata(NamedObject **root, const char *id, unsigned max);
	};

	unsigned max;
	NamedObject **root;

	size_t minsize(unsigned max, size_t ps);

public:
	keyassoc(size_t pagesize = 0, unsigned keysize = 177);
	~keyassoc();

	inline void purge(void)
		{release();};

	void *get(const char *id);
	void set(const char *id, void *data);
	void clear(const char *id);
};

template <class T, size_t S>
class pager : private PagerPool
{
public:
	inline pager() : PagerPool(S, sizeof(T)) {};

	inline T *create(void)
		{return new(get()) T;};
};

template <class T>
class keymem
{
private:
	keyassoc *heap;

public:
	inline keymem(keyassoc *h)
		{heap = h;};

	inline T &get(const char *id)
		{return *(new(heap, id) T);};

	inline T *find(const char *id)
		{return static_cast<T*>(get(id));};

	inline void set(const char *id, const T& v)
		{*(new(heap, id) T) = v;};
};

END_NAMESPACE

void *operator new(size_t size, ucc::mempager *pager);
void *operator new[](size_t size, ucc::mempager *pager);
void *operator new(size_t size, ucc::mempager *pager, size_t overdraft);
void *operator new(size_t size, ucc::keyassoc *pager, const char *id);
void *operator new[](size_t size, ucc::keyassoc *pager, const char *id);

extern "C" {
	void *cpr_mapalloc(size_t size);
	void cpr_mapfree(void *ptr);
	void *cpr_maprealloc(void *ptr, size_t size);
};

#endif
