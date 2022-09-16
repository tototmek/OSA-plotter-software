lista_list = [[2, 7, 6, 34, 1], [2, 7, 56, 34, 1], [2, 5, 6, 4, 1],
              [2, 1, 2, 34, 1]]


def wywal_ponizej_3(lista):
    listaB = []
    for dupa in lista:
        if dupa > 3:
            listaB.append(dupa)
    return listaB


for lista in lista_list:
    print(wywal_ponizej_3(lista))