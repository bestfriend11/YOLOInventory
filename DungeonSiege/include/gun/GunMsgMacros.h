
//////////////////////////////////
//  These are Macros for declaring a Message classes.
//

#define RENDER_DECLARATION                              \
        STDMETHODIMP RenderUnicode(                     \
                        IN LPWSTR pwszDes,              \
                        IN DWORD cchSize,               \
                        OUT DWORD* pcchUsed)const;      \


#define CMD_BASE_ARGS_DEC   PVOID pvContext
#define CMD_BASE_ARGS       pvContext

#define RES_BASE_ARGS_DEC   PVOID pvContext, DWORD dwHRESULT
#define RES_BASE_ARGS       pvContext, dwHRESULT

#define EVT_BASE_ARGS_DEC   DWORD no
#define EVT_BASE_ARGS       no

//
// Command Packets
//
#define COMMAND_0( Msg )                                    \
  MESSAGE_CLASS_0( Msg, Command, CMD_BASE_ARGS_DEC )


#define COMMAND_1( Msg, Type1, Prefix1, Attr1 )             \
  MESSAGE_CLASS_1( Msg, Command, CMD_BASE_ARGS_DEC,         \
                   Type1, Prefix1, Attr1 );


#define COMMAND_2( Msg, Type1, Prefix1, Attr1,              \
                            Type2, Prefix2, Attr2 )         \
  MESSAGE_CLASS_2( Msg, Command, CMD_BASE_ARGS_DEC,         \
                   Type1, Prefix1, Attr1,                   \
                   Type2, Prefix2, Attr2 );


#define COMMAND_3( Msg, Type1, Prefix1, Attr1,              \
                            Type2, Prefix2, Attr2,          \
                            Type3, Prefix3, Attr3 )         \
  MESSAGE_CLASS_3( Msg, Command, CMD_BASE_ARGS_DEC,         \
                   Type1, Prefix1, Attr1,                   \
                   Type2, Prefix2, Attr2,                   \
                   Type3, Prefix3, Attr3 );


#define COMMAND_4( Msg, Type1, Prefix1, Attr1,              \
                            Type2, Prefix2, Attr2,          \
                            Type3, Prefix3, Attr3,          \
                            Type4, Prefix4, Attr4 )         \
  MESSAGE_CLASS_4( Msg, Command, CMD_BASE_ARGS_DEC,         \
                   Type1, Prefix1, Attr1,                   \
                   Type2, Prefix2, Attr2,                   \
                   Type3, Prefix3, Attr3,                   \
                   Type4, Prefix4, Attr4 );


#define COMMAND_5( Msg, Type1, Prefix1, Attr1,              \
                            Type2, Prefix2, Attr2,          \
                            Type3, Prefix3, Attr3,          \
                            Type4, Prefix4, Attr4,          \
                            Type5, Prefix5, Attr5 )         \
  MESSAGE_CLASS_5( Msg, Command, CMD_BASE_ARGS_DEC,         \
                   Type1, Prefix1, Attr1,                   \
                   Type2, Prefix2, Attr2,                   \
                   Type3, Prefix3, Attr3,                   \
                   Type4, Prefix4, Attr4,                   \
                   Type5, Prefix5, Attr5 );


//
// Result Packets.
//
#define RESULT_0( Msg )                                     \
  MESSAGE_CLASS_0( Msg, Result, RES_BASE_ARGS_DEC )


#define RESULT_1( Msg, Type1, Prefix1, Attr1 )              \
  MESSAGE_CLASS_1( Msg, Result, RES_BASE_ARGS_DEC,          \
                   Type1, Prefix1, Attr1 );


#define RESULT_2( Msg, Type1, Prefix1, Attr1,               \
                       Type2, Prefix2, Attr2 )              \
  MESSAGE_CLASS_2( Msg, Result, RES_BASE_ARGS_DEC,          \
                   Type1, Prefix1, Attr1,                   \
                   Type2, Prefix2, Attr2 );

#define RESULT_3( Msg, Type1, Prefix1, Attr1,               \
                       Type2, Prefix2, Attr2,               \
                       Type3, Prefix3, Attr3 );             \
    MESSAGE_CLASS_3( Msg, Result, RES_BASE_ARGS_DEC,        \
                     Type1, Prefix1, Attr1,                 \
                     Type2, Prefix2, Attr2,                 \
                     Type3, Prefix3, Attr3 );   

//
// Event Packets.
//
#define EVENT_0( Msg )                                      \
  MESSAGE_CLASS_0( Msg, Event, EVT_BASE_ARGS_DEC );

#define EVENT_1( Msg, Type1, Prefix1, Attr1 )               \
  MESSAGE_CLASS_1( Msg, Event, EVT_BASE_ARGS_DEC,           \
                   Type1, Prefix1, Attr1 );


#define EVENT_2( Msg, Type1, Prefix1, Attr1,                \
                            Type2, Prefix2, Attr2 )         \
  MESSAGE_CLASS_2( Msg, Event, EVT_BASE_ARGS_DEC,           \
                   Type1, Prefix1, Attr1,                   \
                   Type2, Prefix2, Attr2 );


#define EVENT_3( Msg, Type1, Prefix1, Attr1,                \
                            Type2, Prefix2, Attr2,          \
                            Type3, Prefix3, Attr3 )         \
  MESSAGE_CLASS_3( Msg, Event, EVT_BASE_ARGS_DEC,           \
                   Type1, Prefix1, Attr1,                   \
                   Type2, Prefix2, Attr2,                   \
                   Type3, Prefix3, Attr3 );



// 
// Worker Definitions.
//

#define MESSAGE_CLASS_0( Msg, PktType, BaseArgs )           \
    class C##Msg##PktType: public CBase##PktType            \
    {   public:                                             \
            C##Msg##PktType( IXmlPacket* pXmlPkt );         \
            C##Msg##PktType();                              \
            ~C##Msg##PktType();                             \
            STDMETHODIMP InitFromWire( void );              \
            STDMETHODIMP InitNew( BaseArgs );               \
    private:                                                \
        RENDER_DECLARATION                                  \
    };



#define MESSAGE_CLASS_1( Msg, PktType, BaseArgs,            \
                        Type1, Prefix1, Attr1 )             \
    class C##Msg##PktType: public CBase##PktType            \
    {   public:                                             \
            C##Msg##PktType( IXmlPacket* pXmlPkt );         \
            C##Msg##PktType();                              \
            ~C##Msg##PktType();                             \
            STDMETHODIMP InitFromWire( void );              \
            STDMETHODIMP InitNew( BaseArgs,                 \
                          Type1 Prefix1##Attr1 );           \
    public:                                                 \
        GET_ATTR( Msg, PktType, Type1, Prefix1, Attr1);     \
    private:                                                \
        Type1 m_##Prefix1##Attr1;                           \
    private:                                                \
        RENDER_DECLARATION                                  \
    };


#define MESSAGE_CLASS_2( Msg, PktType, BaseArgs,            \
                        Type1, Prefix1, Attr1,              \
                        Type2, Prefix2, Attr2 )             \
    class C##Msg##PktType: public CBase##PktType            \
    {   public:                                             \
            C##Msg##PktType( IXmlPacket* pXmlPkt );         \
            C##Msg##PktType();                              \
            ~C##Msg##PktType();                             \
            STDMETHODIMP InitFromWire( void );              \
            STDMETHODIMP InitNew( BaseArgs,                 \
                          Type1 Prefix1##Attr1,             \
                          Type2 Prefix2##Attr2 );           \
    public:                                                 \
        GET_ATTR( Msg, PktType, Type1, Prefix1, Attr1);     \
        GET_ATTR( Msg, PktType, Type2, Prefix2, Attr2);     \
    private:                                                \
        Type1 m_##Prefix1##Attr1;                           \
        Type2 m_##Prefix2##Attr2;                           \
    private:                                                \
        RENDER_DECLARATION                                  \
    };


#define MESSAGE_CLASS_3( Msg, PktType, BaseArgs,            \
                        Type1, Prefix1, Attr1,              \
                        Type2, Prefix2, Attr2,              \
                        Type3, Prefix3, Attr3 )             \
    class C##Msg##PktType: public CBase##PktType            \
    {   public:                                             \
            C##Msg##PktType( IXmlPacket* pXmlPkt );         \
            C##Msg##PktType();                              \
            ~C##Msg##PktType();                             \
            STDMETHODIMP InitFromWire( void );              \
            STDMETHODIMP InitNew( BaseArgs,                 \
                          Type1 Prefix1##Attr1,             \
                          Type2 Prefix2##Attr2,             \
                          Type3 Prefix3##Attr3 );           \
    public:                                                 \
        GET_ATTR( Msg, PktType, Type1, Prefix1, Attr1);     \
        GET_ATTR( Msg, PktType, Type2, Prefix2, Attr2);     \
        GET_ATTR( Msg, PktType, Type3, Prefix3, Attr3);     \
    private:                                                \
        Type1 m_##Prefix1##Attr1;                           \
        Type2 m_##Prefix2##Attr2;                           \
        Type3 m_##Prefix3##Attr3;                           \
    private:                                                \
        RENDER_DECLARATION                                  \
    };


#define MESSAGE_CLASS_4( Msg, PktType, BaseArgs,            \
                        Type1, Prefix1, Attr1,              \
                        Type2, Prefix2, Attr2,              \
                        Type3, Prefix3, Attr3,              \
                        Type4, Prefix4, Attr4 )             \
    class C##Msg##PktType: public CBase##PktType            \
    {   public:                                             \
            C##Msg##PktType( IXmlPacket* pXmlPkt );         \
            C##Msg##PktType();                              \
            ~C##Msg##PktType();                             \
            STDMETHODIMP InitFromWire( void );              \
            STDMETHODIMP InitNew( BaseArgs,                 \
                          Type1 Prefix1##Attr1,             \
                          Type2 Prefix2##Attr2,             \
                          Type3 Prefix3##Attr3,             \
                          Type4 Prefix4##Attr4 );           \
    public:                                                 \
        GET_ATTR( Msg, PktType, Type1, Prefix1, Attr1);     \
        GET_ATTR( Msg, PktType, Type2, Prefix2, Attr2);     \
        GET_ATTR( Msg, PktType, Type3, Prefix3, Attr3);     \
        GET_ATTR( Msg, PktType, Type4, Prefix4, Attr4);     \
    private:                                                \
        Type1 m_##Prefix1##Attr1;                           \
        Type2 m_##Prefix2##Attr2;                           \
        Type3 m_##Prefix3##Attr3;                           \
        Type4 m_##Prefix4##Attr4;                           \
    private:                                                \
        RENDER_DECLARATION                                  \
    };


#define MESSAGE_CLASS_5( Msg, PktType, BaseArgs,            \
                        Type1, Prefix1, Attr1,              \
                        Type2, Prefix2, Attr2,              \
                        Type3, Prefix3, Attr3,              \
                        Type4, Prefix4, Attr4,              \
                        Type5, Prefix5, Attr5 )             \
    class C##Msg##PktType: public CBase##PktType            \
    {   public:                                             \
            C##Msg##PktType( IXmlPacket* pXmlPkt );         \
            C##Msg##PktType();                              \
            ~C##Msg##PktType();                             \
            STDMETHODIMP InitFromWire( void );              \
            STDMETHODIMP InitNew( BaseArgs,                 \
                          Type1 Prefix1##Attr1,             \
                          Type2 Prefix2##Attr2,             \
                          Type3 Prefix3##Attr3,             \
                          Type4 Prefix4##Attr4,             \
                          Type5 Prefix5##Attr5 );           \
    public:                                                 \
        GET_ATTR( Msg, PktType, Type1, Prefix1, Attr1);     \
        GET_ATTR( Msg, PktType, Type2, Prefix2, Attr2);     \
        GET_ATTR( Msg, PktType, Type3, Prefix3, Attr3);     \
        GET_ATTR( Msg, PktType, Type4, Prefix4, Attr4);     \
        GET_ATTR( Msg, PktType, Type5, Prefix5, Attr5);     \
    private:                                                \
        Type1 m_##Prefix1##Attr1;                           \
        Type2 m_##Prefix2##Attr2;                           \
        Type3 m_##Prefix3##Attr3;                           \
        Type4 m_##Prefix4##Attr4;                           \
        Type5 m_##Prefix5##Attr5;                           \
    private:                                                \
        RENDER_DECLARATION                                  \
    };



// A result with counted list, plus 0 extra fields.
#define RESULT_CL_0( Msg, Type, Prefix, Name )              \
  MESSAGE_CLASS_CL_0( Msg, Result, RES_BASE_ARGS_DEC,       \
                        Type, Prefix, Name );

#define RESULT_CL_3( Msg, TypeCL, PrefixCL, NameCL,         \
                    Type1, Prefix1, Name1,                  \
                    Type2, Prefix2, Name2,                  \
                    Type3, Prefix3, Name3 )                 \
  MESSAGE_CLASS_CL_3( Msg, Result, RES_BASE_ARGS_DEC,       \
                        TypeCL, PrefixCL, NameCL,           \
                        Type1, Prefix1, Name1,              \
                        Type2, Prefix2, Name2,              \
                        Type3, Prefix3, Name3 )          


#define EVENT_CL_0( Msg, Type, Prefix, Name )               \
  MESSAGE_CLASS_CL_0( Msg, Event, EVT_BASE_ARGS_DEC,        \
                        Type, Prefix, Name );


#define MESSAGE_CLASS_CL_0( Msg, PktType, BaseArgs,         \
                        Type, Prefix, Name )                \
    class C##Msg##PktType: public CBase##PktType            \
    {   public:                                             \
            C##Msg##PktType( IXmlPacket* pXmlPkt );         \
            C##Msg##PktType();                              \
            ~C##Msg##PktType();                             \
            STDMETHODIMP InitFromWire( void );              \
            STDMETHODIMP InitNew( BaseArgs,                 \
                            DWORD dwCount,                  \
                            Type* a##Prefix##Name );        \
    private:                                                \
        STDMETHODIMP Get##Name##FromWire( Type** pa##Prefix##Name,   \
                                  DWORD* pdwCount );        \
        STDMETHODIMP Render##Name(                          \
            LPWSTR pwszDes, DWORD cchSize, DWORD* pcchUsed, \
            Type* a##Prefix##Name, DWORD dwCount ) const;   \
    public:                                                 \
        GET_ATTR( Msg, PktType, DWORD, dw, Count );         \
        GET_ATTR( Msg, PktType, Type*, a##Prefix, Name );   \
    private:                                                \
        DWORD m_dwCount;                                    \
        Type* m_a##Prefix##Name;                            \
    private:                                                \
        RENDER_DECLARATION                                  \
    };


#define MESSAGE_CLASS_CL_3( Msg, PktType, BaseArgs,             \
                        TypeCL, PrefixCL, NameCL,               \
                        Type1, Prefix1, Name1,                  \
                        Type2, Prefix2, Name2,                  \
                        Type3, Prefix3, Name3 )                 \
    class C##Msg##PktType: public CBase##PktType                \
    {   public:                                                 \
            C##Msg##PktType( IXmlPacket* pXmlPkt );             \
            C##Msg##PktType();                                  \
            ~C##Msg##PktType();                                 \
            STDMETHODIMP InitFromWire( void );                  \
            STDMETHODIMP InitNew( BaseArgs,                     \
                            DWORD dwCount,                      \
                            TypeCL* a##PrefixCL##NameCL,        \
                            Type1 Prefix1##Name1,               \
                            Type2 Prefix2##Name2,               \
                            Type3 Prefix3##Name3 );             \
    private:                                                    \
        STDMETHODIMP Get##NameCL##FromWire( TypeCL** pa##PrefixCL##NameCL, \
                                  DWORD* pdwCount );            \
        STDMETHODIMP Render##NameCL(                            \
            LPWSTR pwszDes, DWORD cchSize, DWORD* pcchUsed,     \
            TypeCL* a##PrefixCL##NameCL, DWORD dwCount ) const; \
    public:                                                     \
        GET_ATTR( Msg, PktType, DWORD, dw, Count );             \
        GET_ATTR( Msg, PktType, TypeCL*, a##PrefixCL, NameCL ); \
        GET_ATTR( Msg, PktType, Type1, Prefix1, Name1);         \
        GET_ATTR( Msg, PktType, Type2, Prefix2, Name2);         \
        GET_ATTR( Msg, PktType, Type3, Prefix3, Name3);         \
    private:                                                    \
        DWORD m_dwCount;                                        \
        TypeCL* m_a##PrefixCL##NameCL;                          \
        Type1 m_##Prefix1##Name1;                               \
        Type2 m_##Prefix2##Name2;                               \
        Type3 m_##Prefix3##Name3;                               \
    private:                                                    \
        RENDER_DECLARATION                                      \
    };




#define COMMAND_UTYPE( Msg, Type, Prefix, Name )              \
  MESSAGE_CLASS_UTYPE( Msg, Command, CMD_BASE_ARGS_DEC,       \
                        Type, Prefix, Name );

#define RESULT_UTYPE( Msg, Type, Prefix, Name )              \
  MESSAGE_CLASS_UTYPE( Msg, Result, RES_BASE_ARGS_DEC,       \
                        Type, Prefix, Name );

#define MESSAGE_CLASS_UTYPE( Msg, PktType, BaseArgs,        \
                             Type, Prefix, Name )           \
    class C##Msg##PktType: public CBase##PktType            \
    {   public:                                             \
            C##Msg##PktType( IXmlPacket* pXmlPkt );         \
            C##Msg##PktType();                              \
            ~C##Msg##PktType();                             \
            STDMETHODIMP InitFromWire( void );              \
            STDMETHODIMP InitNew( BaseArgs,                 \
                            Type* Prefix##Name );           \
    private:                                                \
        STDMETHODIMP Get##Name##FromWire( Type** p##Prefix##Name );   \
        STDMETHODIMP Render##Name(                          \
            LPWSTR pwszDes, DWORD cchSize, DWORD* pcchUsed, \
                              Type* Prefix##Name ) const;   \
    public:                                                 \
        GET_ATTR( Msg, PktType, Type*, Prefix, Name );      \
    private:                                                \
        DWORD m_dwCount;                                    \
        Type* m_##Prefix##Name;                             \
    private:                                                \
        RENDER_DECLARATION                                  \
    };


