#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "newt.h"

int main(void) {
    newtComponent b1, b2, b3, b4;
    newtComponent answer, f, t;
    newtGrid grid, subgrid;
    char * flowedText;
    int textWidth, textHeight, rc;
    char * menuContents[] = { "一", "二", "三", "四", "五", NULL };
    char * entries[10];
    struct newtWinEntry autoEntries[] = {
	{ "エントリ", entries + 0, 0 },
	{ "別のエントリ", entries + 1, 0 },
	{ "三番目のエントリ", entries + 2, 0 },
	{ "四番目のエントリ", entries + 3, 0 },
	{ NULL, NULL, 0 } };

    memset(entries, 0, sizeof(entries));

    newtInit();
    newtCls();

    b1 = newtCheckbox(-1, -1, "テストのためのかなり長いチェックボックス", ' ', NULL, NULL);
    b2 = newtButton(-1, -1, "別のボタン");
    b3 = newtButton(-1, -1, "しかし、しかし");
    b4 = newtButton(-1, -1, "しかし何だろう？");

    f = newtForm(NULL, NULL, 0);

    grid = newtCreateGrid(2, 2);
    newtGridSetField(grid, 0, 0, NEWT_GRID_COMPONENT, b1, 0, 0, 0, 0, 
			NEWT_ANCHOR_RIGHT, 0);
    newtGridSetField(grid, 0, 1, NEWT_GRID_COMPONENT, b2, 0, 0, 0, 0, 0, 0);
    newtGridSetField(grid, 1, 0, NEWT_GRID_COMPONENT, b3, 0, 0, 0, 0, 0, 0);
    newtGridSetField(grid, 1, 1, NEWT_GRID_COMPONENT, b4, 0, 0, 0, 0, 0, 0);


    newtFormAddComponents(f, b1, b2, b3, b4, NULL);

    newtGridWrappedWindow(grid, "一番目のウィンドウ");
    newtGridFree(grid, 1);

    answer = newtRunForm(f);
	
    newtFormDestroy(f);
    newtPopWindow();

    flowedText = newtReflowText("これはかなりテキストらしいものです。40カラム"
			  	"の長さで、ラッピングが行われます。"
			  	"素早い、茶色の狐がのろまな犬を飛び"
			  	"越えたのを知ってるかい?\n\n"
				"他にお知らせすることとして、適当に改行をする"
				"ことが重要です。",
				40, 5, 5, &textWidth, &textHeight);
    t = newtTextbox(-1, -1, textWidth, textHeight, NEWT_FLAG_WRAP);
    newtTextboxSetText(t, flowedText);
    free(flowedText);

    
    b1 = newtButton(-1, -1, "了解");
    b2 = newtButton(-1, -1, "キャンセル");

    grid = newtCreateGrid(1, 2);
    subgrid = newtCreateGrid(2, 1);

    newtGridSetField(subgrid, 0, 0, NEWT_GRID_COMPONENT, b1, 0, 0, 0, 0, 0, 0);
    newtGridSetField(subgrid, 1, 0, NEWT_GRID_COMPONENT, b2, 0, 0, 0, 0, 0, 0);

    newtGridSetField(grid, 0, 0, NEWT_GRID_COMPONENT, t, 0, 0, 0, 1, 0, 0);
    newtGridSetField(grid, 0, 1, NEWT_GRID_SUBGRID, subgrid, 0, 0, 0, 0, 0,
			NEWT_GRID_FLAG_GROWX);
    newtGridWrappedWindow(grid, "別の例");
    newtGridDestroy(grid, 1);

    f = newtForm(NULL, NULL, 0);
    newtFormAddComponents(f, b1, t, b2, NULL);
    answer = newtRunForm(f);

    newtPopWindow();
    newtFormDestroy(f);

    newtWinMessage("シンプル", "了解", "これはシンプルなメッセージウィンドウです");
    newtWinChoice("シンプル", "了解", "キャンセル", "これはシンプルな選択ウィンドウです");

    textWidth = 0;
    rc = newtWinMenu("テストメニュー", "これは newtWinMenu() コールのサンプル"
		     "です。 スクロールバーは必要に応じてついたり、 "
		     "つかなかったりします。", 50, 5, 5, 3, 
		     menuContents, &textWidth, "了解", "キャンセル", NULL);

    rc = newtWinEntries("テキスト newtWinEntries()", "これは newtWinEntries()"
		     "コールのサンプルです。たいへん簡単にたくさんの入力を"
		     "扱うことができます。", 50, 5, 5, 20, autoEntries, "了解", 
		     "キャンセル", NULL);

    newtFinished();

    printf("rc = 0x%x item = %d\n", rc, textWidth);

    return 0;
}
