/*
 * RollScheme.h
 *
 *  Created on: Jul 31, 2016
 *      Author: helium
 */

#ifndef SRC_ROLLSCHEME_H_
#define SRC_ROLLSCHEME_H_

#include <iostream>
#include <time.h>
using namespace std;
class RollScheme {
public:
	RollScheme();

	static string from_alias(const string &alias, struct tm &date);
	static double get_ticksize(const string &product);

	static double get_constract_size(const string &product);
	virtual ~RollScheme();
};

#endif /* SRC_ROLLSCHEME_H_ */
