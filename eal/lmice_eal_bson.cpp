#include "lmice_eal_bson.h"

const char * EalBson::GetJsonData()
{
	char * strJson = NULL;
	strJson = bson_as_json(m_ptr_bson, NULL);
	return strJson;
}

int EalBson::AppendArray( const char*key, EalBson *bsonObj )
{
	if( key && bsonObj )
	{
		if( bson_append_array( m_ptr_bson, key, -1, bsonObj->GetObj() ) )
			return m_ptr_bson->len;
	}
	return -1;
}
int EalBson::AppendDocument( const char *key, EalBson *bsonObj )
{
	if( key && bsonObj )
	{
		if( bson_append_document( m_ptr_bson, key, -1, bsonObj->GetObj() ) )
			return m_ptr_bson->len;
	}
	return -1;
}

/*
int EalBson::AppendBson( const char *key, EalBson *bsonObj )
{
	bson_iter_t iter;
	
}*/

int EalBson::AppendBool( const char *key, bool value )
{
	if( key )
	{
		if( bson_append_bool( m_ptr_bson, key, -1, value ) )
			return m_ptr_bson->len;
	}
	return -1;
}


int EalBson::AppendDouble( const char *key, double value )
{
	if( key )
	{
		if( bson_append_double( m_ptr_bson, key, -1, value ) )
			return m_ptr_bson->len;
	}
	return -1;

}

int EalBson::AppendInt32( const char *key, int32_t value )
{
	if( key )
	{
		if( bson_append_int32( m_ptr_bson, key, -1, value ) )
			return m_ptr_bson->len;
	}
	return -1;
}

int EalBson::AppendInt64( const char *key, int64_t value )
{
	if( key )
	{
		if( bson_append_int64( m_ptr_bson, key, -1, value ) )
			return m_ptr_bson->len;
	}
	return -1;
}

int EalBson::AppendNull( const char *key )
{
	if( key )
	{
		if( bson_append_null( m_ptr_bson, key, -1 ) )
			return m_ptr_bson->len;
	}
	return -1;
}

int EalBson::AppendUtf8( const char *key, const char *value )
{
	if( key && value )
	{
		if( bson_append_utf8( m_ptr_bson, key, -1, value, -1 ) )
			return m_ptr_bson->len;
	}
	return -1;
}

int EalBson::AppendSymbol( const char *key, const char *value )
{
	if( key && value )
	{
		if( bson_append_symbol( m_ptr_bson, key, -1, value, -1 ) )
			return m_ptr_bson->len;
	}
	return -1;
}

int EalBson::AppendFlag( const char *key, char value )
{
	if( key )
	{
		if( bson_append_symbol( m_ptr_bson, key, -1, &value, 1 ) )
			return m_ptr_bson->len;
	}
	return -1;
}

int EalBson::AppendTimeT( const char *key, time_t value )
{
	if( key )
	{
		if( bson_append_time_t( m_ptr_bson, key, -1, value ) )
			return m_ptr_bson->len;
	}
	return -1;
}

int EalBson::AppendTimeStamp( const char *key, uint32_t timestamp, uint32_t increment )
{
	uint64_t value = ((((uint64_t)timestamp) << 32) | ((uint64_t)increment));
   	value = BSON_UINT64_TO_LE (value);
		
	if( key )
	{
		if( bson_append_time_t( m_ptr_bson, key, -1, value ) )
			return m_ptr_bson->len;
	}
	return -1;
}

int EalBson::AppendNowUtc( const char *key )
{
	if( key )
	{
		if( bson_append_now_utc( m_ptr_bson, key, -1 ) )
			return m_ptr_bson->len;
	}
	return -1;
}

int EalBson::AppendDateTime( const char *key, int64_t value )
{
	if( key )
	{
		if( bson_append_date_time( m_ptr_bson, key, -1, value ) )
			return m_ptr_bson->len;
	}
	return -1;
}

int EalBson::AppendTimeVal( const char *key, struct timeval *value )
{
	if( key && value )
	{
		if( bson_append_timeval( m_ptr_bson, key, -1, value ) )
			return m_ptr_bson->len;
	}
	return -1;
}

int EalBson::AppendUndefined( const char *key )
{
	if( key )
	{
		if( bson_append_undefined( m_ptr_bson, key, -1 ) )
			return m_ptr_bson->len;
	}
	return -1;
}



