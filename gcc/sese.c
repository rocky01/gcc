/* Single entry single exit control flow regions.
   Copyright (C) 2008, 2009  Free Software Foundation, Inc.
   Contributed by Jan Sjodin <jan.sjodin@amd.com> and
   Sebastian Pop <sebastian.pop@amd.com>.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "ggc.h"
#include "tree.h"
#include "rtl.h"
#include "basic-block.h"
#include "diagnostic.h"
#include "tree-flow.h"
#include "toplev.h"
#include "tree-dump.h"
#include "timevar.h"
#include "cfgloop.h"
#include "tree-chrec.h"
#include "tree-data-ref.h"
#include "tree-scalar-evolution.h"
#include "tree-pass.h"
#include "domwalk.h"
#include "value-prof.h"
#include "pointer-set.h"
#include "gimple.h"
#include "sese.h"

/* Print to stderr the element ELT.  */

static void
debug_rename_elt (rename_map_elt elt)
{
  fprintf (stderr, "(");
  print_generic_expr (stderr, elt->old_name, 0);
  fprintf (stderr, ", ");
  print_generic_expr (stderr, elt->new_name, 0);
  fprintf (stderr, ")\n");
}

/* Helper function for debug_rename_map.  */

static int
debug_rename_map_1 (void **slot, void *s ATTRIBUTE_UNUSED)
{
  struct rename_map_elt *entry = (struct rename_map_elt *) *slot;
  debug_rename_elt (entry);
  return 1;
}

/* Print to stderr all the elements of MAP.  */

void
debug_rename_map (htab_t map)
{
  htab_traverse (map, debug_rename_map_1, NULL);
}

/* Computes a hash function for database element ELT.  */

hashval_t
rename_map_elt_info (const void *elt)
{
  return htab_hash_pointer (((const struct rename_map_elt *) elt)->old_name);
}

/* Compares database elements E1 and E2.  */

int
eq_rename_map_elts (const void *e1, const void *e2)
{
  const struct rename_map_elt *elt1 = (const struct rename_map_elt *) e1;
  const struct rename_map_elt *elt2 = (const struct rename_map_elt *) e2;

  return (elt1->old_name == elt2->old_name);
}



/* Print to stderr the element ELT.  */

static void
debug_ivtype_elt (ivtype_map_elt elt)
{
  fprintf (stderr, "(%s, ", elt->cloog_iv);
  print_generic_expr (stderr, elt->type, 0);
  fprintf (stderr, ")\n");
}

/* Helper function for debug_ivtype_map.  */

static int
debug_ivtype_map_1 (void **slot, void *s ATTRIBUTE_UNUSED)
{
  struct ivtype_map_elt *entry = (struct ivtype_map_elt *) *slot;
  debug_ivtype_elt (entry);
  return 1;
}

/* Print to stderr all the elements of MAP.  */

void
debug_ivtype_map (htab_t map)
{
  htab_traverse (map, debug_ivtype_map_1, NULL);
}

/* Computes a hash function for database element ELT.  */

hashval_t
ivtype_map_elt_info (const void *elt)
{
  return htab_hash_pointer (((const struct ivtype_map_elt *) elt)->cloog_iv);
}

/* Compares database elements E1 and E2.  */

int
eq_ivtype_map_elts (const void *e1, const void *e2)
{
  const struct ivtype_map_elt *elt1 = (const struct ivtype_map_elt *) e1;
  const struct ivtype_map_elt *elt2 = (const struct ivtype_map_elt *) e2;

  return (elt1->cloog_iv == elt2->cloog_iv);
}



/* Debug the list of old induction variables for this SCOP.  */

void
debug_oldivs (sese region)
{
  int i;
  name_tree oldiv;

  fprintf (stderr, "Old IVs:");

  for (i = 0; VEC_iterate (name_tree, SESE_OLDIVS (region), i, oldiv); i++)
    {
      fprintf (stderr, "(");
      print_generic_expr (stderr, oldiv->t, 0);
      fprintf (stderr, ", %s, %d)\n", oldiv->name, oldiv->loop->num);
    }
  fprintf (stderr, "\n");
}

/* Record LOOP as occuring in REGION.  */

static void
sese_record_loop (sese region, loop_p loop)
{
  if (sese_contains_loop (region, loop))
    return;

  bitmap_set_bit (SESE_LOOPS (region), loop->num);
  VEC_safe_push (loop_p, heap, SESE_LOOP_NEST (region), loop);
}

/* Build the loop nests contained in REGION.  Returns true when the
   operation was successful.  */

void
build_sese_loop_nests (sese region)
{
  unsigned i;
  basic_block bb;
  struct loop *loop0, *loop1;

  FOR_EACH_BB (bb)
    if (bb_in_sese_p (bb, region))
      {
	struct loop *loop = bb->loop_father;

	/* Only add loops if they are completely contained in the SCoP.  */
	if (loop->header == bb
	    && bb_in_sese_p (loop->latch, region))
	  sese_record_loop (region, loop);
      }

  /* Make sure that the loops in the SESE_LOOP_NEST are ordered.  It
     can be the case that an inner loop is inserted before an outer
     loop.  To avoid this, semi-sort once.  */
  for (i = 0; VEC_iterate (loop_p, SESE_LOOP_NEST (region), i, loop0); i++)
    {
      if (VEC_length (loop_p, SESE_LOOP_NEST (region)) == i + 1)
	break;

      loop1 = VEC_index (loop_p, SESE_LOOP_NEST (region), i + 1);
      if (loop0->num > loop1->num)
	{
	  VEC_replace (loop_p, SESE_LOOP_NEST (region), i, loop1);
	  VEC_replace (loop_p, SESE_LOOP_NEST (region), i + 1, loop0);
	}
    }
}

/* For a USE in BB, if BB is outside REGION, mark the USE in the
   LIVEOUTS set.  */

static void
sese_build_liveouts_use (sese region, bitmap liveouts, basic_block bb,
			 tree use)
{
  unsigned ver;
  basic_block def_bb;

  if (TREE_CODE (use) != SSA_NAME)
    return;

  ver = SSA_NAME_VERSION (use);
  def_bb = gimple_bb (SSA_NAME_DEF_STMT (use));

  if (!def_bb
      || !bb_in_sese_p (def_bb, region)
      || bb_in_sese_p (bb, region))
    return;

  bitmap_set_bit (liveouts, ver);
}

/* Marks for rewrite all the SSA_NAMES defined in REGION and that are
   used in BB that is outside of the REGION.  */

static void
sese_build_liveouts_bb (sese region, bitmap liveouts, basic_block bb)
{
  gimple_stmt_iterator bsi;
  edge e;
  edge_iterator ei;
  ssa_op_iter iter;
  tree var;

  FOR_EACH_EDGE (e, ei, bb->succs)
    for (bsi = gsi_start_phis (e->dest); !gsi_end_p (bsi); gsi_next (&bsi))
      sese_build_liveouts_use (region, liveouts, bb,
			       PHI_ARG_DEF_FROM_EDGE (gsi_stmt (bsi), e));

  for (bsi = gsi_start_bb (bb); !gsi_end_p (bsi); gsi_next (&bsi))
    FOR_EACH_SSA_TREE_OPERAND (var, gsi_stmt (bsi), iter, SSA_OP_ALL_USES)
      sese_build_liveouts_use (region, liveouts, bb, var);
}

/* Build the LIVEOUTS of REGION: the set of variables defined inside
   and used outside the REGION.  */

static void
sese_build_liveouts (sese region, bitmap liveouts)
{
  basic_block bb;

  FOR_EACH_BB (bb)
    sese_build_liveouts_bb (region, liveouts, bb);
}

/* Builds a new SESE region from edges ENTRY and EXIT.  */

sese
new_sese (edge entry, edge exit)
{
  sese res = XNEW (struct sese);

  SESE_ENTRY (res) = entry;
  SESE_EXIT (res) = exit;
  SESE_LOOPS (res) = BITMAP_ALLOC (NULL);
  SESE_LOOP_NEST (res) = VEC_alloc (loop_p, heap, 3);
  SESE_ADD_PARAMS (res) = true;
  SESE_PARAMS (res) = VEC_alloc (name_tree, heap, 3);
  SESE_OLDIVS (res) = VEC_alloc (name_tree, heap, 3);
  SESE_PARAMS_INDEX (res) = NULL;

  return res;
}

/* Deletes REGION.  */

void
free_sese (sese region)
{
  int i;
  name_tree p, iv;

  if (SESE_LOOPS (region))
    SESE_LOOPS (region) = BITMAP_ALLOC (NULL);

  for (i = 0; VEC_iterate (name_tree, SESE_PARAMS (region), i, p); i++)
    free (p);

  VEC_free (name_tree, heap, SESE_PARAMS (region));
  VEC_free (loop_p, heap, SESE_LOOP_NEST(region));

  for (i = 0; VEC_iterate (name_tree, SESE_OLDIVS (region), i, iv); i++)
    free (iv);

  VEC_free (name_tree, heap, SESE_OLDIVS (region));

  if (SESE_PARAMS_INDEX (region))
    htab_delete (SESE_PARAMS_INDEX (region));

  XDELETE (region);
}

/* Get the index for parameter VAR in SESE.  */

int
parameter_index_in_region (tree var, sese sese)
{
  int i;
  name_tree p;
  name_tree nvar;

  gcc_assert (TREE_CODE (var) == SSA_NAME);

  for (i = 0; VEC_iterate (name_tree, SESE_PARAMS (sese), i, p); i++)
    if (p->t == var)
      return i;

  gcc_assert (SESE_ADD_PARAMS (sese));

  nvar = XNEW (struct name_tree);
  nvar->t = var;
  nvar->name = NULL;
  VEC_safe_push (name_tree, heap, SESE_PARAMS (sese), nvar);
  return VEC_length (name_tree, SESE_PARAMS (sese)) - 1;
}

/* Add exit phis for USE on EXIT.  */

static void
sese_add_exit_phis_edge (basic_block exit, tree use, edge false_e, edge true_e)
{
  gimple phi = create_phi_node (use, exit);

  create_new_def_for (gimple_phi_result (phi), phi,
		      gimple_phi_result_ptr (phi));
  add_phi_arg (phi, use, false_e);
  add_phi_arg (phi, use, true_e);
}

/* Insert in the block BB phi nodes for variables defined in REGION
   and used outside the REGION.  The code generation moves REGION in
   the else clause of an "if (1)" and generates code in the then
   clause that is at this point empty:

   | if (1)
   |   empty;
   | else
   |   REGION;
*/

void
sese_insert_phis_for_liveouts (sese region, basic_block bb,
			       edge false_e, edge true_e)
{
  unsigned i;
  bitmap_iterator bi;
  bitmap liveouts = BITMAP_ALLOC (NULL);

  update_ssa (TODO_update_ssa);

  sese_build_liveouts (region, liveouts);
  EXECUTE_IF_SET_IN_BITMAP (liveouts, 0, i, bi)
    sese_add_exit_phis_edge (bb, ssa_name (i), false_e, true_e);
  BITMAP_FREE (liveouts);

  update_ssa (TODO_update_ssa);
}

/* Get the definition of NAME before the SESE.  Keep track of the
   basic blocks that have been VISITED in a bitmap.  */

static tree
get_vdef_before_sese (sese region, tree name, sbitmap visited)
{
  unsigned i;
  gimple def_stmt = SSA_NAME_DEF_STMT (name);
  basic_block def_bb = gimple_bb (def_stmt);

  if (!def_bb || !bb_in_sese_p (def_bb, region))
    return name;

  if (TEST_BIT (visited, def_bb->index))
    return NULL_TREE;

  SET_BIT (visited, def_bb->index);

  switch (gimple_code (def_stmt))
    {
    case GIMPLE_PHI:
      for (i = 0; i < gimple_phi_num_args (def_stmt); i++)
	{
	  tree arg = gimple_phi_arg_def (def_stmt, i);
	  tree res = get_vdef_before_sese (region, arg, visited);
	  if (res)
	    return res;
	}
      return NULL_TREE;

    default:
      return NULL_TREE;
    }
}

/* Adjust a virtual phi node PHI that is placed at the end of the
   generated code for SCOP:

   | if (1)
   |   generated code from REGION;
   | else
   |   REGION;

   The FALSE_E edge comes from the original code, TRUE_E edge comes
   from the code generated for the SCOP.  */

static void
sese_adjust_vphi (sese region, gimple phi, edge true_e)
{
  unsigned i;

  gcc_assert (gimple_phi_num_args (phi) == 2);

  for (i = 0; i < gimple_phi_num_args (phi); i++)
    if (gimple_phi_arg_edge (phi, i) == true_e)
      {
	tree true_arg, false_arg, before_scop_arg;
	sbitmap visited;

	true_arg = gimple_phi_arg_def (phi, i);
	if (!SSA_NAME_IS_DEFAULT_DEF (true_arg))
	  return;

	false_arg = gimple_phi_arg_def (phi, i == 0 ? 1 : 0);
	if (SSA_NAME_IS_DEFAULT_DEF (false_arg))
	  return;

	visited = sbitmap_alloc (last_basic_block);
	sbitmap_zero (visited);
	before_scop_arg = get_vdef_before_sese (region, false_arg, visited);
	gcc_assert (before_scop_arg != NULL_TREE);
	SET_PHI_ARG_DEF (phi, i, before_scop_arg);
	sbitmap_free (visited);
      }
}

/* Returns the name associated to OLD_NAME in MAP.  */

static tree
get_rename (htab_t map, tree old_name)
{
  struct rename_map_elt tmp;
  PTR *slot;

  tmp.old_name = old_name;
  slot = htab_find_slot (map, &tmp, NO_INSERT);

  if (slot && *slot)
    return ((rename_map_elt) *slot)->new_name;

  return old_name;
}

/* Register in MAP the rename tuple (old_name, new_name).  */

static void
set_rename (htab_t map, tree old_name, tree new_name)
{
  struct rename_map_elt tmp;
  PTR *slot;

  if (old_name == new_name)
    return;

  tmp.old_name = old_name;
  slot = htab_find_slot (map, &tmp, INSERT);

  if (!slot)
    return;

  if (*slot)
    free (*slot);

  *slot = new_rename_map_elt (old_name, new_name);
}

/* Adjusts the phi nodes in the block BB for variables defined in
   SCOP_REGION and used outside the SCOP_REGION.  The code generation
   moves SCOP_REGION in the else clause of an "if (1)" and generates
   code in the then clause:

   | if (1)
   |   generated code from REGION;
   | else
   |   REGION;

   To adjust the phi nodes after the condition, the RENAME_MAP is
   used.  */

void
sese_adjust_liveout_phis (sese region, htab_t rename_map, basic_block bb,
			  edge false_e, edge true_e)
{
  gimple_stmt_iterator si;

  for (si = gsi_start_phis (bb); !gsi_end_p (si); gsi_next (&si))
    {
      unsigned i;
      unsigned false_i = 0;
      gimple phi = gsi_stmt (si);

      if (!is_gimple_reg (PHI_RESULT (phi)))
	{
	  sese_adjust_vphi (region, phi, true_e);
	  continue;
	}

      for (i = 0; i < gimple_phi_num_args (phi); i++)
	if (gimple_phi_arg_edge (phi, i) == false_e)
	  {
	    false_i = i;
	    break;
	  }

      for (i = 0; i < gimple_phi_num_args (phi); i++)
	if (gimple_phi_arg_edge (phi, i) == true_e)
	  {
	    tree old_name = gimple_phi_arg_def (phi, false_i);
	    tree new_name = get_rename (rename_map, old_name);

	    gcc_assert (old_name != new_name);
	    SET_PHI_ARG_DEF (phi, i, new_name);
	  }
    }
}

/* Return the old induction variable of the LOOP that is in normal
   form in REGION.  */

tree
oldiv_for_loop (sese region, loop_p loop)
{
  int i;
  name_tree iv;

  if (!loop)
    return NULL_TREE;

  for (i = 0; VEC_iterate (name_tree, SESE_OLDIVS (region), i, iv); i++)
    if (iv->loop == loop)
      return iv->t;

  return NULL_TREE;
}

/* Rename the SSA_NAMEs used in STMT and that appear in MAP.  */

static void 
rename_variables_in_stmt (gimple stmt, htab_t map)
{
  ssa_op_iter iter;
  use_operand_p use_p;

  FOR_EACH_SSA_USE_OPERAND (use_p, stmt, iter, SSA_OP_USE)
    {
      tree use = USE_FROM_PTR (use_p);
      tree new_name = get_rename (map, use);

      replace_exp (use_p, new_name);
    }

  update_stmt (stmt);
}

/* Returns true if SSA_NAME is a parameter of SESE.  */

static bool
is_parameter (sese region, tree ssa_name)
{
  int i;
  VEC (name_tree, heap) *params = SESE_PARAMS (region);
  name_tree param;

  for (i = 0; VEC_iterate (name_tree, params, i, param); i++)
    if (param->t == ssa_name)
      return true;

  return false;
}

/* Returns true if NAME is an induction variable.  */

static bool
is_iv (tree name)
{
  return gimple_code (SSA_NAME_DEF_STMT (name)) == GIMPLE_PHI;
}

static void expand_scalar_variables_stmt (gimple, basic_block, sese,
					  htab_t, gimple_stmt_iterator *);
static tree
expand_scalar_variables_expr (tree, tree, enum tree_code, tree, basic_block,
			      sese, htab_t, gimple_stmt_iterator *);

/* Copies at GSI all the scalar computations on which the ssa_name OP0
   depends on in the SESE: these are all the scalar variables used in
   the definition of OP0, that are defined outside BB and still in the
   SESE, i.e. not a parameter of the SESE.  The expression that is
   returned contains only induction variables from the generated code:
   MAP contains the induction variables renaming mapping, and is used
   to translate the names of induction variables.  */

static tree
expand_scalar_variables_ssa_name (tree op0, basic_block bb,
				  sese region, htab_t map, 
				  gimple_stmt_iterator *gsi)
{
  tree var0, var1, type;
  gimple def_stmt;
  enum tree_code subcode;
  tree new_op;
      
  if (is_parameter (region, op0)
      || is_iv (op0))
    return get_rename (map, op0);
      
  def_stmt = SSA_NAME_DEF_STMT (op0);

  /* We already created this stmt in this bb.  */
  new_op = get_rename (map, op0);

  if (new_op != op0
      && gimple_bb (SSA_NAME_DEF_STMT (new_op)) == bb)
    return new_op;
      
  if (gimple_bb (def_stmt) == bb)
    {
      /* If the defining statement is in the basic block already
	 we do not need to create a new expression for it, we
	 only need to ensure its operands are expanded.  */
      expand_scalar_variables_stmt (def_stmt, bb, region, map, gsi);
      return new_op;
    }
  else
    {
      if (gimple_code (def_stmt) != GIMPLE_ASSIGN
	  || !bb_in_sese_p (gimple_bb (def_stmt), region))
	return new_op;

      var0 = gimple_assign_rhs1 (def_stmt);
      subcode = gimple_assign_rhs_code (def_stmt);
      var1 = gimple_assign_rhs2 (def_stmt);
      type = gimple_expr_type (def_stmt);

      return expand_scalar_variables_expr (type, var0, subcode, var1, bb,
					   region, map, gsi);
    }
}

/* Copies at GSI all the scalar computations on which the expression
   OP0 CODE OP1 depends on in the SESE: these are all the scalar
   variables used in OP0 and OP1, defined outside BB and still defined
   in the SESE, i.e. not a parameter of the SESE.  The expression that
   is returned contains only induction variables from the generated
   code: MAP contains the induction variables renaming mapping, and is
   used to translate the names of induction variables.  */

static tree
expand_scalar_variables_expr (tree type, tree op0, enum tree_code code, 
			      tree op1, basic_block bb, sese region, 
			      htab_t map, gimple_stmt_iterator *gsi)
{
  if (TREE_CODE_CLASS (code) == tcc_constant
      || TREE_CODE_CLASS (code) == tcc_declaration)
    return op0;

  /* For data references we have to duplicate also its memory
     indexing.  */
  if (TREE_CODE_CLASS (code) == tcc_reference)
    {
      switch (code)
	{
	case REALPART_EXPR:
	case IMAGPART_EXPR:
	  {
	    tree op = TREE_OPERAND (op0, 0);
	    tree res = expand_scalar_variables_expr
	      (type, op, TREE_CODE (op), NULL, bb, region, map, gsi);
	    return build1 (code, type, res);
	  }

	case INDIRECT_REF:
	  {
	    tree old_name = TREE_OPERAND (op0, 0);
	    tree expr = expand_scalar_variables_ssa_name
	      (old_name, bb, region, map, gsi);
	    tree new_name = force_gimple_operand_gsi (gsi, expr, true, NULL,
						      true, GSI_SAME_STMT);

	    set_symbol_mem_tag (SSA_NAME_VAR (new_name),
				symbol_mem_tag (SSA_NAME_VAR (old_name)));
	    return fold_build1 (code, type, new_name);
	  }

	case ARRAY_REF:
	  {
	    tree op00 = TREE_OPERAND (op0, 0);
	    tree op01 = TREE_OPERAND (op0, 1);
	    tree op02 = TREE_OPERAND (op0, 2);
	    tree op03 = TREE_OPERAND (op0, 3);
	    tree base = expand_scalar_variables_expr
	      (TREE_TYPE (op00), op00, TREE_CODE (op00), NULL, bb, region,
	       map, gsi);
	    tree subscript = expand_scalar_variables_expr
	      (TREE_TYPE (op01), op01, TREE_CODE (op01), NULL, bb, region,
	       map, gsi);

	    return build4 (ARRAY_REF, type, base, subscript, op02, op03);
	  }

	default:
	  /* The above cases should catch everything.  */
	  gcc_unreachable ();
	}
    }

  if (TREE_CODE_CLASS (code) == tcc_unary)
    {
      tree op0_type = TREE_TYPE (op0);
      enum tree_code op0_code = TREE_CODE (op0);
      tree op0_expr = expand_scalar_variables_expr (op0_type, op0, op0_code,
						    NULL, bb, region, map, gsi);
  
      return fold_build1 (code, type, op0_expr);
    }

  if (TREE_CODE_CLASS (code) == tcc_binary
      || TREE_CODE_CLASS (code) == tcc_comparison)
    {
      tree op0_type = TREE_TYPE (op0);
      enum tree_code op0_code = TREE_CODE (op0);
      tree op0_expr = expand_scalar_variables_expr (op0_type, op0, op0_code,
						    NULL, bb, region, map, gsi);
      tree op1_type = TREE_TYPE (op1);
      enum tree_code op1_code = TREE_CODE (op1);
      tree op1_expr = expand_scalar_variables_expr (op1_type, op1, op1_code,
						    NULL, bb, region, map, gsi);

      return fold_build2 (code, type, op0_expr, op1_expr);
    }

  if (code == SSA_NAME)
    return expand_scalar_variables_ssa_name (op0, bb, region, map, gsi);

  if (code == ADDR_EXPR)
    return op0;

  gcc_unreachable ();
  return NULL;
}

/* Copies at the beginning of BB all the scalar computations on which
   STMT depends on in the SESE: these are all the scalar variables used
   in STMT, defined outside BB and still defined in the SESE, i.e. not a
   parameter of the SESE.  The expression that is returned contains
   only induction variables from the generated code: MAP contains the
   induction variables renaming mapping, and is used to translate the
   names of induction variables.  */
 
static void
expand_scalar_variables_stmt (gimple stmt, basic_block bb, sese region,
			      htab_t map, gimple_stmt_iterator *gsi)
{
  ssa_op_iter iter;
  use_operand_p use_p;

  FOR_EACH_SSA_USE_OPERAND (use_p, stmt, iter, SSA_OP_USE)
    {
      tree use = USE_FROM_PTR (use_p);
      tree type = TREE_TYPE (use);
      enum tree_code code = TREE_CODE (use);
      tree use_expr = expand_scalar_variables_expr (type, use, code, NULL, bb,
						    region, map, gsi);
      if (use_expr != use)
	{
	  tree new_use =
	    force_gimple_operand_gsi (gsi, use_expr, true, NULL,
				      true, GSI_SAME_STMT);
	  replace_exp (use_p, new_use);
	}
    }

  update_stmt (stmt);
}

/* Copies at the beginning of BB all the scalar computations on which
   BB depends on in the SESE: these are all the scalar variables used
   in BB, defined outside BB and still defined in the SESE, i.e. not a
   parameter of the SESE.  The expression that is returned contains
   only induction variables from the generated code: MAP contains the
   induction variables renaming mapping, and is used to translate the
   names of induction variables.  */

static void 
expand_scalar_variables (basic_block bb, sese region, htab_t map)
{
  gimple_stmt_iterator gsi;
  
  for (gsi = gsi_after_labels (bb); !gsi_end_p (gsi);)
    {
      gimple stmt = gsi_stmt (gsi);
      expand_scalar_variables_stmt (stmt, bb, region, map, &gsi);
      gsi_next (&gsi);
    }
}

/* Rename all the SSA_NAMEs from block BB according to the MAP.  */

static void 
rename_variables (basic_block bb, htab_t map)
{
  gimple_stmt_iterator gsi;
  
  for (gsi = gsi_after_labels (bb); !gsi_end_p (gsi); gsi_next (&gsi))
    rename_variables_in_stmt (gsi_stmt (gsi), map);
}

/* Remove condition from BB.  */

static void
remove_condition (basic_block bb)
{
  gimple last = last_stmt (bb);

  if (last && gimple_code (last) == GIMPLE_COND)
    {
      gimple_stmt_iterator gsi = gsi_last_bb (bb);
      gsi_remove (&gsi, true);
    }
}

/* Returns the first successor edge of BB with EDGE_TRUE_VALUE flag set.  */

edge
get_true_edge_from_guard_bb (basic_block bb)
{
  edge e;
  edge_iterator ei;

  FOR_EACH_EDGE (e, ei, bb->succs)
    if (e->flags & EDGE_TRUE_VALUE) 
      return e;

  gcc_unreachable ();
  return NULL;
}

/* Returns the first successor edge of BB with EDGE_TRUE_VALUE flag cleared.  */

edge
get_false_edge_from_guard_bb (basic_block bb)
{
  edge e;
  edge_iterator ei;

  FOR_EACH_EDGE (e, ei, bb->succs)
    if (!(e->flags & EDGE_TRUE_VALUE)) 
      return e;

  gcc_unreachable ();
  return NULL;
}

/* Returns true when NAME is defined in LOOP.  */

static bool
defined_in_loop_p (tree name, loop_p loop)
{
  gimple stmt = SSA_NAME_DEF_STMT (name);

  return (gimple_bb (stmt)->loop_father == loop);
}

/* Returns the gimple statement that uses NAME outside the loop it is
   defined in, returns NULL if there is no such loop close phi node.
   An invariant of the loop closed SSA form is that the only use of a
   variable, outside the loop it is defined in, is in the loop close
   phi node that just follows the loop.  */

static gimple
alive_after_loop (tree name)
{
  use_operand_p use_p;
  imm_use_iterator imm_iter;
  loop_p loop = gimple_bb (SSA_NAME_DEF_STMT (name))->loop_father;

  FOR_EACH_IMM_USE_FAST (use_p, imm_iter, name)
    {
      gimple stmt = USE_STMT (use_p);

      if (gimple_code (stmt) == GIMPLE_PHI
	  && gimple_bb (stmt)->loop_father != loop)
	return stmt;
    }

  return NULL;
}

/* Return true if a close phi has not yet been inserted for the use of
   variable NAME on the single exit of LOOP.  */

static bool
close_phi_not_yet_inserted_p (loop_p loop, tree name)
{
  gimple_stmt_iterator psi;
  basic_block bb = single_exit (loop)->dest;

  for (psi = gsi_start_phis (bb); !gsi_end_p (psi); gsi_next (&psi))
    if (gimple_phi_arg_def (gsi_stmt (psi), 0) == name)
      return false;

  return true;
}

/* A structure for passing parameters to add_loop_exit_phis.  */

typedef struct alep {
  loop_p loop;
  VEC (rename_map_elt, heap) *new_renames;
} *alep_p;

/* Helper function for htab_traverse in insert_loop_close_phis.  */

static int
add_loop_exit_phis (void **slot, void *data)
{
  struct rename_map_elt *entry;
  alep_p a;
  loop_p loop;
  tree new_name;
  bool def_in_loop_p, used_outside_p, need_close_phi_p;
  gimple old_close_phi;

  if (!slot || !data)
    return 1;

  entry = (struct rename_map_elt *) *slot;
  a = (alep_p) data;
  loop = a->loop;
  new_name = entry->new_name;

  if (TREE_CODE (new_name) != SSA_NAME)
    return 1;

  def_in_loop_p = defined_in_loop_p (new_name, loop);
  old_close_phi = alive_after_loop (entry->old_name);
  used_outside_p = (old_close_phi != NULL);
  need_close_phi_p = (def_in_loop_p && used_outside_p
		      && close_phi_not_yet_inserted_p (loop, new_name));

  /* Insert a loop close phi node.  */
  if (need_close_phi_p)
    {
      basic_block bb = single_exit (loop)->dest;
      gimple phi = create_phi_node (new_name, bb);
      tree new_res = create_new_def_for (gimple_phi_result (phi), phi,
					 gimple_phi_result_ptr (phi));

      add_phi_arg (phi, new_name, single_pred_edge (bb));
      VEC_safe_push (rename_map_elt, heap, a->new_renames,
		     new_rename_map_elt (gimple_phi_result (old_close_phi),
					 new_res));
    }

  /* Remove the old rename from the map.  */
  if (def_in_loop_p && *slot)
    {
      free (*slot);
      *slot = NULL;
    }

  return 1;
}

/* Traverses MAP and removes from it all the tuples (OLD, NEW) where
   NEW is defined in LOOP.  Inserts on the exit of LOOP the close phi
   node "RES = phi (NEW)" corresponding to "OLD_RES = phi (OLD)" in
   the original code.  Inserts in MAP the tuple (OLD_RES, RES).  */

void
insert_loop_close_phis (htab_t map, loop_p loop)
{
  int i;
  struct alep a;
  rename_map_elt elt;

  a.loop = loop;
  a.new_renames = VEC_alloc (rename_map_elt, heap, 3);
  update_ssa (TODO_update_ssa);
  htab_traverse (map, add_loop_exit_phis, &a);
  update_ssa (TODO_update_ssa);

  for (i = 0; VEC_iterate (rename_map_elt, a.new_renames, i, elt); i++)
    {
      set_rename (map, elt->old_name, elt->new_name);
      free (elt);
    }

  VEC_free (rename_map_elt, heap, a.new_renames);
}

/* Helper structure for htab_traverse in insert_guard_phis.  */

struct igp {
  basic_block bb;
  edge true_edge, false_edge;
  htab_t before_guard;
};

/* Return the default name that is before the guard.  */

static tree
default_before_guard (htab_t before_guard, tree old_name)
{
  tree res = get_rename (before_guard, old_name);

  if (res == old_name)
    {
      if (is_gimple_reg (res))
	return fold_convert (TREE_TYPE (res), integer_zero_node);
      return gimple_default_def (cfun, res);
    }

  return res;
}

/* Helper function for htab_traverse in insert_guard_phis.  */

static int
add_guard_exit_phis (void **slot, void *s)
{
  struct rename_map_elt *entry = (struct rename_map_elt *) *slot;
  struct igp *i = (struct igp *) s;
  basic_block bb = i->bb;
  edge true_edge = i->true_edge;
  edge false_edge = i->false_edge;
  tree name1 = entry->new_name;
  tree name2 = default_before_guard (i->before_guard, entry->old_name);
  gimple phi;
  tree res;

  /* Nothing to be merged if the name before the guard is the same as
     the one after.  */
  if (name1 == name2)
    return 1;

  phi = create_phi_node (name1, bb);
  res = create_new_def_for (gimple_phi_result (phi), phi,
			    gimple_phi_result_ptr (phi));

  add_phi_arg (phi, name1, true_edge);
  add_phi_arg (phi, name2, false_edge);

  entry->new_name = res;
  *slot = entry;
  return 1;
}

/* Iterate over RENAME_MAP and get tuples of the form (OLD, NAME1).
   If there is a correspondent tuple (OLD, NAME2) in BEFORE_GUARD,
   with NAME1 different than NAME2, then insert in BB the phi node:
   
   | RES = phi (NAME1 (on TRUE_EDGE), NAME2 (on FALSE_EDGE))"

   if there is no tuple for OLD in BEFORE_GUARD, insert

   | RES = phi (NAME1 (on TRUE_EDGE),
   |            DEFAULT_DEFINITION of NAME1 (on FALSE_EDGE))".

   Finally register in RENAME_MAP the tuple (OLD, RES).  */

void
insert_guard_phis (basic_block bb, edge true_edge, edge false_edge,
		   htab_t before_guard, htab_t rename_map)
{
  struct igp i;
  i.bb = bb;
  i.true_edge = true_edge;
  i.false_edge = false_edge;
  i.before_guard = before_guard;

  update_ssa (TODO_update_ssa);
  htab_traverse (rename_map, add_guard_exit_phis, &i);
  update_ssa (TODO_update_ssa);
}

/* Create a duplicate of the basic block BB.  NOTE: This does not
   preserve SSA form.  */

static void
graphite_copy_stmts_from_block (basic_block bb, basic_block new_bb, htab_t map)
{
  gimple_stmt_iterator gsi, gsi_tgt;

  gsi_tgt = gsi_start_bb (new_bb);
  for (gsi = gsi_start_bb (bb); !gsi_end_p (gsi); gsi_next (&gsi))
    {
      def_operand_p def_p;
      ssa_op_iter op_iter;
      int region;
      gimple stmt = gsi_stmt (gsi);
      gimple copy;

      if (gimple_code (stmt) == GIMPLE_LABEL)
	continue;

      /* Create a new copy of STMT and duplicate STMT's virtual
	 operands.  */
      copy = gimple_copy (stmt);
      gsi_insert_after (&gsi_tgt, copy, GSI_NEW_STMT);
      mark_symbols_for_renaming (copy);

      region = lookup_stmt_eh_region (stmt);
      if (region >= 0)
	add_stmt_to_eh_region (copy, region);
      gimple_duplicate_stmt_histograms (cfun, copy, cfun, stmt);

      /* Create new names for all the definitions created by COPY and
	 add replacement mappings for each new name.  */
      FOR_EACH_SSA_DEF_OPERAND (def_p, copy, op_iter, SSA_OP_DEF)
	{
	  tree old_name = DEF_FROM_PTR (def_p);
	  tree new_name = create_new_def_for (old_name, copy, def_p);
	  set_rename (map, old_name, new_name);
	}
    }
}

/* Copies BB and includes in the copied BB all the statements that can
   be reached following the use-def chains from the memory accesses,
   and returns the next edge following this new block.  */
 
edge
copy_bb_and_scalar_dependences (basic_block bb, sese region,
				edge next_e, htab_t map)
{
  basic_block new_bb = split_edge (next_e);

  next_e = single_succ_edge (new_bb);
  graphite_copy_stmts_from_block (bb, new_bb, map);
  remove_condition (new_bb);
  remove_phi_nodes (new_bb);
  expand_scalar_variables (new_bb, region, map);
  rename_variables (new_bb, map);

  return next_e;
}

/* Returns the outermost loop in SCOP that contains BB.  */

struct loop *
outermost_loop_in_sese (sese region, basic_block bb)
{
  struct loop *nest;

  nest = bb->loop_father;
  while (loop_outer (nest)
	 && loop_in_sese_p (loop_outer (nest), region))
    nest = loop_outer (nest);

  return nest;
}

/* Sets the false region of an IF_REGION to REGION.  */

void
if_region_set_false_region (ifsese if_region, sese region)
{
  basic_block condition = if_region_get_condition_block (if_region);
  edge false_edge = get_false_edge_from_guard_bb (condition);
  basic_block dummy = false_edge->dest;
  edge entry_region = SESE_ENTRY (region);
  edge exit_region = SESE_EXIT (region);
  basic_block before_region = entry_region->src;
  basic_block last_in_region = exit_region->src;
  void **slot = htab_find_slot_with_hash (current_loops->exits, exit_region,
					  htab_hash_pointer (exit_region),
					  NO_INSERT);

  entry_region->flags = false_edge->flags;
  false_edge->flags = exit_region->flags;

  redirect_edge_pred (entry_region, condition);
  redirect_edge_pred (exit_region, before_region);
  redirect_edge_pred (false_edge, last_in_region);
  redirect_edge_succ (false_edge, single_succ (dummy));
  delete_basic_block (dummy);

  exit_region->flags = EDGE_FALLTHRU;
  recompute_all_dominators ();

  SESE_EXIT (region) = false_edge;
  if_region->false_region = region;

  if (slot)
    {
      struct loop_exit *loop_exit = GGC_CNEW (struct loop_exit);

      memcpy (loop_exit, *((struct loop_exit **) slot), sizeof (struct loop_exit));
      htab_clear_slot (current_loops->exits, slot);

      slot = htab_find_slot_with_hash (current_loops->exits, false_edge,
				       htab_hash_pointer (false_edge),
				       INSERT);
      loop_exit->e = false_edge;
      *slot = loop_exit;
      false_edge->src->loop_father->exits->next = loop_exit;
    }
}

/* Creates an IFSESE with CONDITION on edge ENTRY.  */

ifsese
create_if_region_on_edge (edge entry, tree condition)
{
  edge e;
  edge_iterator ei;
  sese sese_region = GGC_NEW (struct sese);
  sese true_region = GGC_NEW (struct sese);
  sese false_region = GGC_NEW (struct sese);
  ifsese if_region = GGC_NEW (struct ifsese);
  edge exit = create_empty_if_region_on_edge (entry, condition);

  if_region->region = sese_region;
  if_region->region->entry = entry;
  if_region->region->exit = exit;

  FOR_EACH_EDGE (e, ei, entry->dest->succs)
    {
      if (e->flags & EDGE_TRUE_VALUE)
	{
	  true_region->entry = e;
	  true_region->exit = single_succ_edge (e->dest);
	  if_region->true_region = true_region;
	}
      else if (e->flags & EDGE_FALSE_VALUE)
	{
	  false_region->entry = e;
	  false_region->exit = single_succ_edge (e->dest);
	  if_region->false_region = false_region;
	}
    }

  return if_region;
}

/* Moves REGION in a condition expression:
   | if (1)
   |   ;
   | else
   |   REGION;
*/

ifsese
move_sese_in_condition (sese region)
{
  basic_block pred_block = split_edge (SESE_ENTRY (region));
  ifsese if_region = NULL;

  SESE_ENTRY (region) = single_succ_edge (pred_block);
  if_region = create_if_region_on_edge (single_pred_edge (pred_block), integer_one_node);
  if_region_set_false_region (if_region, region);

  return if_region;
}

