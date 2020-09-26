#pragma once


#define KTRUserPtr KTRUser*
#undef KTRUserPtr
#define KSkTRUserPtr KSkTRUser*
#undef KSkTRUserPtr

SmartPointer( KTRUser );
SmartPointer( KSkTRUser );

class KSkTRUser : public KSocketObject 
{
public:
    enum
    {
        MINIMUM_PACKET_SIZE = 3,
        MAXIMUM_PACKET_SIZE = 512
    };
    void OnRecvCompleted( DWORD dwTransfered );
    void OnSocketError();
    //void OnAcceptConnection() { m_pkTRUser->OnAcceptConnection(); }
    KTRUserPtr GetTRUserPtr();
    boost::weak_ptr< KTRUser > m_pkTRUser;
};

class KTRUser : public KThread, public boost::enable_shared_from_this<KTRUser>
{
	NiDeclareRootRTTI( KTRUser );   // Accepter���� ó���� ��ü���� Ni-RTTI�� ����Ǿ� �־�� �Ѵ�.

public:
	enum ENUM_TICKS // Tick ����
	{
		CONNECT_TICK = 0,
		HB_TICK,            // heart_bit
		TICKS_NUM,
	};

	KTRUser(void);                              // do not use.
	KTRUser(const KTRUser& right);              // do not use.
	KTRUser& operator=(const KTRUser& right);   // do not use.
	virtual ~KTRUser(void);

	virtual bool Begin();   //derived From KThread
			void Tick();

	virtual bool Init();
	virtual bool Connect( const char* szIP, unsigned short usPort, const UidType& iUnitUID );

	UidType			GetUID()const       { return m_iUnitUID; }

	//bool	Send( IN const std::vector<UidType> vecUID, IN const void* pvData, IN const size_t dwDataSize );
	template< typename T >
	bool			SendPacket( unsigned short usEventID, T& data, bool bCompress = true );
	bool			SendID( unsigned short usEventID, bool bCompress = true );

	void			ReserveDestroy() { m_bDestroyReserved = true; }

	bool			IsConnected() const { _JIF( m_spSockObj, return false ); return m_spSockObj->IsConnected(); }
	const char*		GetIPStr() const	{ _JIF( m_spSockObj, return NULL ); return m_spSockObj->GetIPStr(); }
	unsigned short	GetPort() const		{ _JIF( m_spSockObj, return 0 ); return m_spSockObj->GetPort(); }

	void			SetClassID( int iClassID )	{ m_iClassID = iClassID; }  // ������ �޼����� �����Ҷ� ����� id �Ҵ�.
	int				GetClassID()				{ return m_iClassID; }
	void			SetServerMsgID( UINT serverMsgID ){ m_ServerMsgID = serverMsgID; }
	void			SetSendMsgCallBack( SEND_MSG_FUNC pSendGameMessage ){ m_pSendGameMessage = pSendGameMessage; }
    KSkTRUserPtr	GetSkTRUserPtr()        { return m_spSockObj; }

    void	OnSocketError();
protected:
	virtual	void	Run();


	void	SetTick( IN ENUM_TICKS eIndex )	{ m_adwTickCount[ eIndex ] = ::GetTickCount(); }
	DWORD	GetTick( IN ENUM_TICKS eIndex)	{ return m_adwTickCount[ eIndex ]; }

	//void	OnRecvCompleted( IN const char cRecvCnt, IN KSerBuffer& buff );
	void	OnRecvCompleted( IN KSerBuffer& buffer );
	void	OnDestroy();

protected:
    KSkTRUserPtr        m_spSockObj;
	bool                m_bUseIocp;
	bool				m_bDestroyReserved;
	UidType             m_iUnitUID;
	DWORD               m_adwTickCount[KTRUser::TICKS_NUM];
	SEND_MSG_FUNC		m_pSendGameMessage;
	UINT				m_ServerMsgID;
	int					m_iClassID;         // �̺�Ʈ�� ������ �޼����� ���� ��, ������ ������ �Ѵ�.

	// proxy-chile only.
	enum ENUM_EVENT_TYPE 
	{    
		EVENT_DISCONNECTED,
		EVENT_RECV_COMPLETED,        
		EVENT_SEND_COMPLETED,        
		EVENT_MAX_VALUE
	};
	HANDLE              m_hEvents[EVENT_MAX_VALUE];

    friend class KSkTRUser;
};

template<typename T>
bool KTRUser::SendPacket( unsigned short usEventID, T& data_, bool bCompress_ /* = true  */)
{
	if( m_bDestroyReserved ) return true;///< ���ᰡ ����Ǿ����� send �õ�. �̷� ��찡 �ٺ��ϹǷ�, true ����.
	_JIF( m_spSockObj, return false );
    _JIF( m_spSockObj->IsConnected(), return false );    ///< ������ ��ȿ���� ����.

	KEvent kEvent;
	kEvent.SetData( 0, NULL, usEventID, data_ );

	if( bCompress_ ) kEvent.m_kbuff.Compress();

	// serialize - full event object
	KSerializer     ks;
	KSerBuffer      kbuff;
	ks.BeginWriting( &kbuff );
	ks.Put( kEvent );
	ks.EndWriting();

	KncSecurity::KByteStream    bsbuff;
	bsbuff.reserve( kbuff.GetLength() + sizeof(USHORT) );
	bsbuff.Assign( kbuff.GetLength() + sizeof(USHORT), sizeof(USHORT) );
	bsbuff.append( (byte*)kbuff.GetData(), kbuff.GetLength() );   // ���� ������ ������ �ֱ�.

	if( kEvent.m_usEventID != E_HEART_BEAT )
		dbg::clog << L"�ߨ� " << kEvent.GetIDStr() << L" - TR SEND" << END_LOG;

	return m_spSockObj->SendData( (const char*)bsbuff.data(), bsbuff.length() );
}