#include "minifigures.h"

#ifndef MINIFIGS
#define MINIFIGS

Minifigure minifigures[] = {
    {1, "Batman"},
    {2, "Gandalf"},
    {3, "Wyldstyle"},
    {4, "Aquaman"},
    {5, "Bad Cop"},
    {6, "Bane"},
    {7, "Bart Simpson"},
    {8, "Benny"},
    {9, "Chell"},
    {10, "Cole"},
    {11, "Cragger"},
    {12, "Cyborg"},
    {13, "Cyberman"},
    {14, "Doc Brown"},
    {15, "The Doctor"},
    {16, "Emmet"},
    {17, "Eris"},
    {18, "Gimli"},
    {19, "Gollum"},
    {20, "Harley Quinn"},
    {21, "Homer Simpson"},
    {22, "Jay"},
    {23, "Joker"},
    {24, "Kai"},
    {25, "ACU Trooper"},
    {26, "Gamer Kid"},
    {27, "Krusty the Clown"},
    {28, "Laval"},
    {29, "Legolas"},
    {30, "Lloyd"},
    {31, "Marty McFly"},
    {32, "Nya"},
    {33, "Owen Grady"},
    {34, "Peter Venkman"},
    {35, "Slimer"},
    {36, "Scooby-Doo"},
    {37, "Sensei Wu"},
    {38, "Shaggy"},
    {39, "Stay Puft"},
    {40, "Superman"},
    {41, "Unikitty"},
    {42, "Wicked Witch of the West"},
    {43, "Wonder Woman"},
    {44, "Zane"},
    {45, "Green Arrow"},
    {46, "Supergirl"},
    {47, "Abby Yates"},
    {48, "Finn the Human"},
    {49, "Ethan Hunt"},
    {50, "Lumpy Space Princess"},
    {51, "Jake the Dog"},
    {52, "Harry Potter"},
    {53, "Lord Voldemort"},
    {54, "Michael Knight"},
    {55, "B.A. Baracus"},
    {56, "Newt Scamander"},
    {57, "Sonic the Hedgehog"},
    // {58, "Future Update (unreleased)"},
    {59, "Gizmo"},
    {60, "Stripe"},
    {61, "E.T."},
    {62, "Tina Goldstein"},
    {63, "Marceline the Vampire Queen"},
    {64, "Batgirl"},
    {65, "Robin"},
    {66, "Sloth"},
    {67, "Hermione Granger"},
    {68, "Chase McCain"},
    {69, "Excalibur Batman"},
    {70, "Raven"},
    {71, "Beast Boy"},
    {72, "Betelgeuse"},
    // {73, "Lord Vortech (unreleased)"},
    {74, "Blossom"},
    {75, "Bubbles"},
    {76, "Buttercup"},
    {77, "Starfire"},
};

#endif

#include "furi.h"

const char* get_minifigure_name(int id) {
    for(int i = 0; minifigures[i].name != NULL; i++) {
        if(minifigures[i].id == id) {
            return minifigures[i].name;
        }
    }
    return "?";
}

// int get_minifigure_id(const char* name) {
//     for(int i = 0; minifigures[i].name != NULL; i++) {
//         if(strcmp(minifigures[i].name, name) == 0) {
//             return minifigures[i].id;
//         }
//     }
//     return -1;
// }