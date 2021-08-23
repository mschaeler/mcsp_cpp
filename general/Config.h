//
// Created by Martin on 30.07.2021.
//

#ifndef MY_MCSP_CONFIG_H
#define MY_MCSP_CONFIG_H

#endif //MY_MCSP_CONFIG_H

class Config{
public:
    /**
	 * By default set to false. This way every DBMS returns a tid list of all surviving tuples upon selection.
	 * If set true the databases only count how many tuples survive, i.e., execute select count(*) from T where [predicates]
	 *
	 * It needs to be static and final such that the compiler only keeps the desired implementation in the binaries.
	 */
    const static bool COUNT_ONLY = false;
    /**
	 * If set true the read and write costs are recorded and displayed in the benchmark.
	 * Note, displaying costs may have considerable negative impact on run time and it may affect DBMSs differently.
	 */
    const static bool LOG_COST   = true;
    /**
	 * Defines whether the DBMSs materialize each not jet materialized data set on HDD.
	 * Note, in case the data set is already on HDD, it is not materialized again.
	 */
    const static bool MATERIALIZE_DATA = false;
};
