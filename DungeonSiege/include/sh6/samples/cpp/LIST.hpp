// Example doubly-linked list link class, with each link storing a single
// character string value.
class Link
{
	friend class List;		// allow List class to access private members
public:
	char *Value() const { return value; }
	Link *Next() const { return next; }
	Link *Prev() const { return prev; }
protected:
	Link(MEM_POOL dataPool, char const *val, Link *prev, Link *next);
	Link() { }
	~Link();
	void *operator new(size_t, MEM_POOL linkPool)
		{ return MemAllocFS(linkPool); }
	void *operator new(size_t) { return NULL; }
	void operator delete(void *mem) { MemFreeFS(mem); }

	Link *next;
	Link *prev;
	char *value;
};

// Example doubly-linked list class, with objects of the above class as links.
// Note that data storage is entirely private to the list: links are allocated
// from fixed-size memory pool member linkPool, while strings are allocated
// from variable-size pool dataPool.
class List
{
public:
	List(unsigned);
	~List();

	Link *Insert(Link *prev, char *val);
	Link *Append(char *val) { return Insert(tail, val); } // add link at end
	Link *Push(char *val) {	return Insert(NULL, val); } // add at beginning
	BOOL Delete(Link *remove);
	BOOL DeleteValue(char *val) 
		{ return Delete(findValue(val)); }
	BOOL DeleteOccurrences(char *val);
	BOOL Pop() { return Delete(head); } // remove first link
	Link *Find(char *val) const 
		{ return findValue(val); }
	long Position(char *val) const;
	Link *First() const { return head; }
	Link *Last() const { return tail; }
	Link *Nth(long n) const;
	long Length() const { return count; }
	BOOL Reverse();
	BOOL Check() const;
	DWORD Shrink();

protected:
	Link *findValue(char *val, long *pos = NULL) const;

	Link *head;
	Link *tail;
	MEM_POOL linkPool;
	MEM_POOL dataPool;
	long count;
};
