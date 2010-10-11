(*
 * $Id: disasm.ml,v 1.16 2009/07/29 09:26:28 casse Exp $
 * Copyright (c) 2008, IRIT - UPS <casse@irit.fr>
 *
 * This file is part of OGliss.
 *
 * OGliss is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * OGliss is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OGliss; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *)

(* return the mask giving the nth param (counting from 0) of an instr of the given spec sp, the result will be a string
with only '0' or '1' chars representing the bits of the mask,
the params' order is the one given by the Iter.get_params method *)
let get_string_mask_for_param_from_op sp n =
	let get_length_from_format f =
		let l = String.length f in
		let new_f =
			if l<=2 then
			(* shouldn't happen, we should have only formats like %[0-9]*b, not %d or %f *)
				"0"
			else
				String.sub f 1 (l-2)
		in
		Scanf.sscanf new_f "%d" (fun x->x)
	in
	let remove_space s =
		let rec concat_str_list s_l =
			match s_l with
			[] ->
				""
			| h::q ->
				h ^ (concat_str_list q)
		in
		concat_str_list (Str.split (Str.regexp "[ \t]+") s)
	in
	let rec change_i_th_param l i =
		match l with
		[] ->
			""
		| a::b ->
			(match a with
			Str.Delim(d) ->
				if i = 0 then
					(String.make (get_length_from_format d) '1') ^ (change_i_th_param b (i-1))
				else
					(String.make (get_length_from_format d) '0') ^ (change_i_th_param b (i-1))
			| Str.Text(txt) ->
				(String.make (String.length txt) '0') ^ (change_i_th_param b i)
			)
	in
	let get_expr_from_iter_value v  =
		match v with
		Iter.EXPR(e) ->
			e
		| _ -> Irg.NONE
	in
	let rec get_frmt_params e =
		match e with
		Irg.FORMAT(_, params) ->
			params
		| Irg.ELINE(_, _, e) ->
			get_frmt_params e
		| _ ->
			failwith "(Decode) can't find the params of a given (supposed) format expr"
	in
	let image_attr =
		get_expr_from_iter_value (Iter.get_attr sp "image")
	in
	let frmt_params =
		get_frmt_params image_attr
	in
	let rec get_str e =
		match e with
		| Irg.FORMAT(str, _) -> str
		| Irg.CONST(t_e, c) ->
			if t_e=Irg.STRING then
				match c with
				Irg.STRING_CONST(str, false, _) ->
					str
				| _ -> ""
			else
				""
		| Irg.ELINE(_, _, e) -> get_str e
		| _ -> ""
	in
	let str_params =
		remove_space (get_str image_attr)
	in
	let rec get_name_of_param e =
		match e with
		Irg.FIELDOF(_, ee, _) ->
			ee
		| Irg.REF(name) ->
			name
		| Irg.ELINE (_, _, ee) ->
			get_name_of_param ee
		| _ ->
			(* !!DEBUG!! *)
			(*print_string "trouble with:";
			Irg.print_expr e;*)
			failwith "(Decode) bad parameter in image format"
	in
	let get_rank_of_named_param n =
		let rec aux nn i p_l =
			match p_l with
			[] ->
				failwith ("(Decode) can't find rank of param "^nn^" in the format params")
			| a::b ->
				if nn=(get_name_of_param a) then
					(* !!DEBUG!! *)
					(*begin
					Printf.printf "get_ranked_of_named_param:[[%d]]\n" i;*)
					
					i
					(* !!DEBUG!! *)
					(*end*)
				else
					aux nn (i+1) b
		in
		aux n 0 frmt_params
	in
	let rec get_i_th_param_name i l =
		match l with
		[] ->
			failwith "(Decode) can't find name of i_th param of a spec"
		| a::b ->
			if i=0 then
				(* !!DEBUG!! *)
				(*begin
				print_string ("get_i_th_param_name:[["^(Irg.get_name_param a)^"]]\n");
				Irg.print_spec sp;*)
				
				Irg.get_name_param a
				(* !!DEBUG!! *)
				(*end*)
			else
				get_i_th_param_name (i-1) b
	in
	(* !!DEBUG!! *)
	(*Irg.print_spec sp;
	print_string ("[["^str_params^"]]\n");
	Irg.print_param_list (Iter.get_params sp);
	print_string "n="; Printf.printf "%d\n" n;*)
	
	change_i_th_param (Str.full_split (Str.regexp "%[0-9]*[bdfxs]") (str_params)) (get_rank_of_named_param (get_i_th_param_name n (Iter.get_params sp)))
