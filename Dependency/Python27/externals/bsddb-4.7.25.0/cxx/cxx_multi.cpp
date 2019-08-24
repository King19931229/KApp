/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997,2008 Oracle.  All rights reserved.
 *
 * $Id: cxx_multi.cpp 63573 2008-05-23 21:43:21Z trent.nelson $
 */

#include "db_config.h"

#include "db_int.h"

#include "db_cxx.h"

DbMultipleIterator::DbMultipleIterator(const Dbt &dbt)
 : data_((u_int8_t*)dbt.get_data()),
   p_((u_int32_t*)(data_ + dbt.get_ulen() - sizeof(u_int32_t)))
{
}

bool DbMultipleDataIterator::next(Dbt &data)
{
	if (*p_ == (u_int32_t)-1) {
		data.set_data(0);
		data.set_size(0);
		p_ = 0;
	} else {
		data.set_data(data_ + *p_--);
		data.set_size(*p_--);
		if (data.get_size() == 0 && data.get_data() == data_)
			data.set_data(0);
	}
	return (p_ != 0);
}

bool DbMultipleKeyDataIterator::next(Dbt &key, Dbt &data)
{
	if (*p_ == (u_int32_t)-1) {
		key.set_data(0);
		key.set_size(0);
		data.set_data(0);
		data.set_size(0);
		p_ = 0;
	} else {
		key.set_data(data_ + *p_--);
		key.set_size(*p_--);
		data.set_data(data_ + *p_--);
		data.set_size(*p_--);
	}
	return (p_ != 0);
}

bool DbMultipleRecnoDataIterator::next(db_recno_t &recno, Dbt &data)
{
	if (*p_ == (u_int32_t)0) {
		recno = 0;
		data.set_data(0);
		data.set_size(0);
		p_ = 0;
	} else {
		recno = *p_--;
		data.set_data(data_ + *p_--);
		data.set_size(*p_--);
	}
	return (p_ != 0);
}
