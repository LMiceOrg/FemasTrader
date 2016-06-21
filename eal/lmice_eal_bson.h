#ifndef LMIC_EAL_BSON_H
#define LMIC_EAL_BSON_H

//#include "include/bson.h"
#include <bson.h>

using namespace std;

class EalBson
{

public:
	EalBson()
	{
		m_ptr_bson = new bson_t;
		bson_init(m_ptr_bson);
		m_strJson = NULL;
	}

	~EalBson()
	{
		if( m_ptr_bson )
		{
			bson_reinit(m_ptr_bson);
			bson_destroy(m_ptr_bson);
		}
	}

//get bson object info
public:
	//get json string of this bson obj
	const char *GetJsonData();
	void FreeJsonData();

	//get bson data buf
	const uint8_t *GetBsonData()
	{
		if( m_ptr_bson->len > 0 )
		{
			return bson_get_data(m_ptr_bson);
		}
		return NULL;
	}

	//get bson data len
	int GetLen()
	{
		return m_ptr_bson->len;
	}

	bson_t *GetObj()
	{
		return m_ptr_bson;
	}

//function: insert bson value
//para    : 1 key, [2 value]
//return  : <0 failed, >0 current bson buf length 

public:	
	int AppendArray( const char*key, EalBson *bsonObj );
	int AppendDocument( const char *key, EalBson *bsonObj );
	int AppendBool( const char *key, bool value );
	int AppendDouble( const char *key, double value );
	int AppendInt32( const char *key, int32_t value );
	int AppendInt64( const char *key, int64_t value );
	int AppendNull( const char *key );
	int AppendUtf8( const char *key, const char *value );
	int AppendSymbol( const char *key, const char *value );
	int AppendFlag( const char *key, char value );
	int AppendTimeT( const char *key, time_t value );
	int AppendTimeStamp( const char *key, uint32_t timestamp, uint32_t increment );
	int AppendNowUtc( const char *key );
	int AppendDateTime( const char *key, int64_t value );
	int AppendTimeVal( const char *key, struct timeval *value );
	int AppendUndefined( const char *key );

	
private:
	bson_t *m_ptr_bson;
	char * m_strJson;
};

#endif // LMIC_EAL_BSON_H

