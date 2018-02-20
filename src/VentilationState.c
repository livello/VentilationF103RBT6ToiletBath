////
//// Created by livello on 19.02.18.
////
//
//#include <stddef.h>
//#include "VentilationState.h"
//#include <stdio.h>
//
//struct state;
//typedef void state_fn(struct state *);
//
//struct state
//{
//    state_fn * next;
//    int i; // data
//};
//
//state_fn foo, bar;
//
//void foo(struct state * state)
//{
//    printf("%s %i\n", __func__, ++state->i);
//    state->next = bar;
//}
//
//void bar(struct state * state)
//{
//    printf("%s %i\n", __func__, ++state->i);
//    state->next = state->i < 10 ? foo : 0;
//}
//
//int main(void)
//{
//    struct state state = { foo, 0 };
//    while(state.next) state.next(&state);
//}