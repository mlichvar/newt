#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "newt.h"

int main(void) {
    newtGrid grid;
    newtComponent checktree;
    newtComponent button;
    newtComponent form;
    newtComponent answer;
    void ** result, **ptr;
    int numselected, i, j;
    int * list;
    
    newtInit();
    newtCls();

    checktree = newtCheckboxTreeMulti(-1, -1, 10, " ab", NEWT_FLAG_SCROLL);
    newtCheckboxTreeAddItem(checktree, "ナンバー", (void *) 2, 0,
    			    NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "本当に本当に長いモノ",
			   (void *) 3, 0, NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "ナンバー５", (void *) 5, 
    			    NEWT_FLAG_SELECTED, 
    			    NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "ナンバー６", (void *) 6, 0, 
    			    NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "ナンバー７", (void *) 7, 
    			    NEWT_FLAG_SELECTED, 
    			    NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "ナンバー８", (void *) 8, 0, 
    			    NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "ナンバー９", (void *) 9, 0, 
    			    NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "ナンバー１０", (void *) 10,
    			    NEWT_FLAG_SELECTED,
    			    NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "ナンバー１１", (void *) 11, 0, 
    			    NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "ナンバー１２", (void *) 12,
    			    NEWT_FLAG_SELECTED,
    			    NEWT_ARG_APPEND, NEWT_ARG_LAST);

    newtCheckboxTreeAddItem(checktree, "カラー", (void *) 1, 0,
    			    0, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "赤色", (void *) 100, 0,
    			    0, NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "白色", (void *) 101, 0,
    			    0, NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "青色", (void *) 102, 0,
    			    0, NEWT_ARG_APPEND, NEWT_ARG_LAST);

    newtCheckboxTreeAddItem(checktree, "ナンバー４", (void *) 4, 0,
    			    3, NEWT_ARG_LAST);

    newtCheckboxTreeAddItem(checktree, "一桁の数字", (void *) 200, 0,
    			    1, NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "一", (void *) 201, 0,
    			    1, 0, NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "二", (void *) 202, 0,
    			    1, 0, NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "三", (void *) 203, 0,
    			    1, 0, NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "四", (void *) 204, 0,
    			    1, 0, NEWT_ARG_APPEND, NEWT_ARG_LAST);

    newtCheckboxTreeAddItem(checktree, "二桁の数字", (void *) 300, 0,
    			    1, NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "十", (void *) 210, 0,
    			    1, 1, NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "十一", (void *) 211, 0,
    			    1, 1, NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "十二", (void *) 212, 0,
    			    1, 1, NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "十三", (void *) 213, 0,
    			    1, 1, NEWT_ARG_APPEND, NEWT_ARG_LAST);

    button = newtButton(-1, -1, "終了");
    
    grid = newtCreateGrid(1, 2);
    newtGridSetField(grid, 0, 0, NEWT_GRID_COMPONENT, checktree, 0, 0, 0, 1, 
		     NEWT_ANCHOR_RIGHT, 0);
    newtGridSetField(grid, 0, 1, NEWT_GRID_COMPONENT, button, 0, 0, 0, 0, 
		     0, 0);

    newtGridWrappedWindow(grid, "チェックボックスツリーテスト");
    newtGridFree(grid, 1);

    form = newtForm(NULL, NULL, 0);
    newtFormAddComponents(form, checktree, button, NULL);

    answer = newtRunForm(form);

    newtFinished();

    result = newtCheckboxTreeGetSelection(checktree, &numselected);
    ptr = result;
    if (!result || !numselected)
	printf("none selected\n");
    else
	printf("Current selection (all) (%d):\n", numselected);
    for (i = 0; i < numselected; i++) {
	j = (int) *ptr++;
	printf("%d\n", j);
    }
    result = newtCheckboxTreeGetMultiSelection(checktree, &numselected, 'b');
    ptr = result;
    if (!result || !numselected)
	printf("none selected\n");
    else
	printf("Current selection (b) (%d):\n",numselected);
    for (i = 0; i < numselected; i++) {
	j = (int) *ptr++;
	printf("%d\n", j);
    }
	
    if (result)
	free(result);

    list = newtCheckboxTreeFindItem(checktree, (void *) 213);
    printf("path:");
    for (i = 0; list && list[i] != NEWT_ARG_LAST; i++)
        printf(" %d", list[i]);
    printf("\n");
    
    newtFormDestroy(form);
    
    return 0;
}
