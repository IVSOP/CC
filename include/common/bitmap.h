#ifndef BITMAP_H
#define BITMAP_H

#include <bitset>
#include <vector>

// NOTA
// cansei de tentar usar std::bitset porque tamanho tem de ser conhecido SEMPRE em compile-time
// nao encontrei nenhum outro bitset menos mal feito
// por alguma razao, std::vector<bool> usa um bitmap internamente,
// mas e dinamico e nao estatico. ja nem quero saber

struct bitMap {
	std::vector<bool> data;
};


#endif
