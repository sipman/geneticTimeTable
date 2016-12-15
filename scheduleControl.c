

int fitness(individual ind){
	return 0;
}

/**
 * Counts conflicts of rooms and teachers in a single individuals
 * @param ind				individuals
 * @param classCount amount of classes
 */
void conflicts(individual *ind, int classCount){
	int class1,   class2,
	    lecture1, lecture2,
	    day,      hour, 
	    tempFlags = 0,
	    conflicts = 0;
	
	for (class1 = 0; class1 < classCount; class1++){
		qsort(&ind->t[class1], MAX_LECTURES, sizeof(lecture), dayHourQsort);
	}


	for (class1 = 0; class1 < classCount - 1; class1++){
		/* Reset conflicts */

		for(lecture1 = 0; lecture1 < ind->t[class1].lectureLength; lecture1++){
			/* Don't check empty lectures */
			if(!ind->t[class1].lectures[lecture1].init){
				continue;
			}

			day  = ind->t[class1].lectures[lecture1].l_datetime.dayOfWeek;
	        hour = ind->t[class1].lectures[lecture1].l_datetime.hour;

           	/* Reset this lecture's conflict flags */
            ind->t[class1].lectures[lecture1].conflictFlag = 0;
            
			for(class2 = class1 + 1; class2 < classCount; class2++){

				/* Foreach lecture in other classes where day is less or equal to the day */
				for(
					lecture2 = 0; 
					lecture2 < ind->t[class2].lectureLength &&
						ind->t[class2].lectures[lecture2].l_datetime.dayOfWeek <= day;
					lecture2++
				){
					tempFlags = 0;
					/* If same day and hour */
					if(ind->t[class2].lectures[lecture2].l_datetime.dayOfWeek == day && 
							ind->t[class2].lectures[lecture2].l_datetime.hour == hour){

						/* If same room */
            			if(ind->t[class2].lectures[lecture2].l_room == ind->t[class1].lectures[lecture1].l_room){
            				tempFlags += ROOM_CONFLICT;
							conflicts++;
            				
            				if(ind->t[class1].lectures[lecture1].conflictFlag < TEACHER_CONFLICT){

            					ind->t[class1].lectures[lecture1].conflictFlag += ROOM_CONFLICT;
								ind->t[class1].numOfConflicts++;
            				}
            				 
            			}

						/* If same teacher */
						if(ind->t[class2].lectures[lecture2].l_teacher == ind->t[class1].lectures[lecture1].l_teacher){
            				tempFlags += TEACHER_CONFLICT;
            				conflicts++;

            				if(ind->t[class1].lectures[lecture1].conflictFlag < TEACHER_CONFLICT){

            					ind->t[class1].lectures[lecture1].conflictFlag += TEACHER_CONFLICT;
            					ind->t[class1].numOfConflicts++;
            				}
            			}

            			if(ind->t[class1].lectures[lecture1].conflictFlag < tempFlags){
            				ind->t[class1].lectures[lecture1].conflictFlag = tempFlags;	
            			}

            			ind->t[class2].lectures[lecture2].conflictFlag = tempFlags;
            			
					}
				}
	        }
		}
	}

	ind->conflicts = conflicts;
}

/**
 * Returns count of dublicates in an array
 * @param	a		 array to check in
 * @param	items how many entries in array
 * @param	size	size of each entry
 * @return			 returns amount of dublicates
 */
int dublicateCount(const void *items, const size_t numberOfItems, const size_t itemSize){
	/* TODO: https://codereview.stackexchange.com/questions/149602/duplicate-counter-in-c */
	int i,j;
	int dublicates = 0;
		char *x = (char *)items;

		for (i = 0; i < numberOfItems - 1; i++){
			for (j = i + 1; j < numberOfItems; j++){
				/* Check if chunks are equal, if so count */
				if(memcmp(&(x[i*itemSize]), &(x[j*itemSize]), itemSize) == 0){
					dublicates++;
				}
			}
		}

		return dublicates;
}

/**
 * Function for qSort to sort for lowest conflicts
 * @param	a first individuals to compare
 * @param	b second individuals to compare
 * @return	 whether a should come first or b
 */
int conflictsQsort(const void * a, const void * b){
	const individual *oa = a;
	const individual *ob = b;

	return (ob->fitness - oa->fitness);
}


individual randomIndividual(params *populationParams){
	int c,day,hour,s;
	int subjectIndex = 0;
	individual r_individual;
	lecture r_lecture; /* variable til at midlertidig gemme random genereret lektion, indtil de bliver placerer i et klasseskema */
	int *hoursPerWeek;
	hoursPerWeek = calloc(populationParams->subjectCount, sizeof(int)); /* intierer arrayet således at der er plads til alle fag */
	if(hoursPerWeek == NULL){
		printf("Not enough ram, sorry...\n");
		exit(EXIT_FAILURE);
	}
	memset(&r_individual,'\0',sizeof(individual));

	/* For hvert individ op til maks antal individer */
	for (c = 0; c < populationParams->classCount; c++){
		r_individual.t[c].forClass = &populationParams->classes[c];
		/* Get all the required hours for class */
		for (s = 0; s < populationParams->subjectCount; s++){
		    hoursPerWeek[s] = ceil(
		    	populationParams->subjects[s].perYear[populationParams->classes[c].year] / ((float)SCHOOL_DAYS_YEAR / (float)WEEK_LENGTH)
		    );
		  /*  if(strcmp(classes[c].name, "1B") == 0 && hoursPerWeek[s] > 0){
		    	 printf("%s skal have %d antal timer i %s om ugen hvilket er %d om året\n", classes[c].name, hoursPerWeek[s], subjects[s].name, subjects[s].perYear[classes[c].year]);
			}*/
		}

		while(!isEmpty(hoursPerWeek,populationParams->subjectCount)){
    		day = randomNumber(0,WEEK_LENGTH-1);
    		hour = randomNumber(0,MAX_LECTURES/WEEK_LENGTH-1);
        	subjectIndex = randomNumber(0,populationParams->subjectCount-1);
        	if(hoursPerWeek[subjectIndex] > 0 && lectureOnDateTime(r_individual.t[c], day, hour) < 0){
	            r_lecture = randomLectureForClassAndSubject(
	            	populationParams,
	            	&populationParams->classes[c],
	            	&populationParams->subjects[subjectIndex]
	            );
	            r_lecture.l_datetime.dayOfWeek = day;
	            r_lecture.l_datetime.hour = hour;
	            r_lecture.init = 1;
	            /*r_individual.t[c].lectureLength += 1;*/
	            hoursPerWeek[subjectIndex] -= 1;
	            r_individual.t[c].lectures[r_individual.t[c].lectureLength++] = r_lecture;
	        }
		}
		qsort(&r_individual.t[c], MAX_LECTURES, sizeof(lecture), dayHourQsort);
	}

	conflicts(&r_individual,populationParams->classCount);

	free(hoursPerWeek);
	return r_individual;
}

int dayHourQsort(const void * a, const void * b){
	const lecture *oa = a;
	const lecture *ob = b;

	if(oa->l_datetime.dayOfWeek != ob->l_datetime.dayOfWeek){
		return oa->l_datetime.dayOfWeek - ob->l_datetime.dayOfWeek;
	}else{
		return oa->l_datetime.hour - ob->l_datetime.hour;
	}
}

int lectureOnDateTime(timetable t, int day, int hour){
	int l;
	for (l = 0; l < MAX_LECTURES; l++){
		if(t.lectures[l].init == 1 && t.lectures[l].l_datetime.dayOfWeek == day && t.lectures[l].l_datetime.hour == hour){
			return l;
		}
	}

	return -1;
}
