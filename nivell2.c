

void internal_print_prompt(char ** args){
	/*
	 *
	 * ha de fer servir el USER i el PWD (env variables)
	 *
	 *
	 */
}

void internal_cd(char ** args){
	/*
	 *
	 * chdir no actualitza PWD
	 *
	 * Podem fer servir getcwd(), que mou al nou directori i actualitza o 
	 * fer manualment un setenv() per actualitzar PWD.
	 *
	 *
	 *
	 *
	 *
	 *
	 */
}

void internal_export(char ** args){
	/*
	 *
	 * export USER=UIB
	 *
	 * 2 arguments, el segon argument té tot el contingut 
	 * podem er un strtok dividint pel delim '='. Després feim setenv()
	 *
	 *
	 *
	 *
	 *
	 *
	 */
}
