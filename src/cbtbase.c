#include "../include/ast.h"
#include "../include/bth_alloc.h"
#include "../include/types.h"

#include "./cbtbase/add.c"
#include "./cbtbase/mul.c"

// trait Add<Rhs, Output>
// where
//     Rhs: Any
//     Ouput: Any
// begin
//     Output add(self, Rhs rhs);
// end
// 

void __define_char_type(void)
{
    TypeInfo *ti = empty_typeinfo();

    ti->repr.id = VT_CHAR;
    ti->traits = bth_htab_new(4, 1, 1);

    define_type("Char", ti);
    __impl_basic_add(ti, ti, ti);
}

void __define_int_type(void)
{
    TypeInfo *ti = empty_typeinfo();

    ti->repr.id = VT_INT;
    ti->traits = bth_htab_new(4, 1, 1);

    define_type("Int", ti);
    __impl_basic_add(ti, ti, ti);
    __impl_basic_mul(ti, ti, ti);

    TypeInfo *vt_char = get_id_type_info(VT_CHAR);
    __impl_basic_add(ti, vt_char, ti);
    // __impl_basic_mul(ti, vt_char, ti);
}

void __define_any_type(void)
{

}

// void define_static_traits(void)
// {
// }

// void define_static_types(void)
// {
// }

void init_cbtbase(void)
{
    TypeInfo *ti = empty_typeinfo();

    ti->repr.id = VT_ANY;
    ti->traits = bth_htab_new(4, 1, 1);

    define_type("Any", ti);

    __define_add_trait();
    __define_mul_trait();

    __define_any_type();
    __define_char_type();
    __define_int_type();
}
