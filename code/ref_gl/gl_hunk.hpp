/*
=============================================================

Hunk allocation

=============================================================
*/

#pragma once

void*	Hunk_Begin(int maxsize);
void*	Hunk_Alloc(int size);
int		Hunk_End(void);
void	Hunk_Free(void *base);
