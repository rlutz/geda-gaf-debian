;;; gEDA - GPL Electronic Design Automation
;;; gnetlist - gEDA Netlist
;;; Copyright (C) 1998-2010 Ales Hvezda
;;; Copyright (C) 1998-2019 gEDA Contributors (see ChangeLog for details)
;;;
;;; This program is free software; you can redistribute it and/or modify
;;; it under the terms of the GNU General Public License as published by
;;; the Free Software Foundation; either version 2 of the License, or
;;; (at your option) any later version.
;;;
;;; This program is distributed in the hope that it will be useful,
;;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;; GNU General Public License for more details.
;;;
;;; You should have received a copy of the GNU General Public License
;;; along with this program; if not, write to the Free Software
;;; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
;;; MA 02111-1301 USA.

;;; --------------------------------------------------------------------------
;;;
;;;  VHDL-AMS netlist backend written by Eduard Moser and Martin Lehmann.
;;;  Build on the VHDL backend from Magnus Danielson
;;;
;;; --------------------------------------------------------------------------

(use-modules (srfi srfi-1))

;;; ===================================================================================
;;;                  TOP LEVEL FUNCTION
;;;                        BEGIN

;;;   Write structural VAMS representation of the schematic

;;;   absolutly toplevel function of gEDA gnelist vams mode.
;;;   its evaluate things like output-file, generate-mode, top-attribs
;;;   and starts the major subroutines.

(define vams
  (lambda (output-filename)
    (let* (
           ;; generate correctly architecture name
           (architecture (vams:change-all-whitespaces-to-underlines
                          (cond
                           ((string=?
                             (gnetlist:get-toplevel-attribute "architecture")
                             "not found") "default_architecture")
                           (else
                            (gnetlist:get-toplevel-attribute "architecture")))))

           ;; generate correctly entity name
           (entity (vams:change-all-whitespaces-to-underlines
                    (cond ((string=?
                            (gnetlist:get-toplevel-attribute "entity")
                            "not found")
                           "default_entity")
                          (else (gnetlist:get-toplevel-attribute "entity")))))

           ;; search all ports of a schematic. for entity generation only.
           (port-list  (vams:generate-port-list (vams:get-uref top-attribs)))

           ;; search all generic of a schematic. for entity generatin only.
           (generic-list (vams:generate-generic-list top-attribs)))


      ;; generate-mode : 1 (default) -> generate a architecture (netlist) of a
      ;;                                schematic
      ;;                 2           -> is selected a component then generate
      ;;                                a entity of this, else generate
      ;;                                a toplevel entity. called from gschem
      ;;                                normally.

      (cond ((= generate-mode 1)
             (begin
               (message "\ngenerating architecture of current schematic in ")

               ;; generate output-filename, like
               ;; (<entity>_arc.<output-file-extension>)
               (if (not (gnetlist:stdout? output-filename))
                 (set! output-filename
                   (string-append
                     (dirname output-filename)
                     "/"
                     (string-downcase! entity)
                     "_arc"
                     (substring output-filename
                                (string-rindex output-filename #\. 0
                                               (string-length output-filename))
                                (string-length output-filename)))))

               (set-current-output-port (gnetlist:output-port output-filename))
               (message output-filename)
               (message "\n")
               (display "-- Structural VAMS generated by gnetlist\n")
               (vams:write-secondary-unit architecture entity)
               (close-output-port (current-output-port))))

            ((= generate-mode 2)
             (message "\n\ngenerating entity of current schematic in ")

             ;; if one component selected, then generate output-filename
             ;; (<device of selected component>.vhdl), else
             ;; <entity>.vhdl
            (if (not (gnetlist:stdout? output-filename))
             (if (not (null? top-attribs))
                 (set! output-filename
                   (string-append
                     (dirname output-filename)
                     "/"
                     (string-downcase!
                       (get-device (vams:get-uref top-attribs)))
                     ".vhdl"))
                 (set! output-filename
                       (string-append
                         (dirname output-filename)
                         "/"
                         (string-downcase! entity)
                         ".vhdl"))))

             (message output-filename)
             (message "\n")
             (set-current-output-port (gnetlist:output-port output-filename))

             ;; decide about the right parameters for entity-declaration
             (if (not (null? (vams:get-uref top-attribs)))
                 (vams:write-primary-unit (get-device (vams:get-uref top-attribs))
                                          port-list
                                          generic-list)
                 (vams:write-primary-unit  entity port-list generic-list))

             (close-output-port (current-output-port)))))))


;;;                  TOP LEVEL FUNCTION
;;;                        END

;;; ===================================================================================

;;;
;;;              ENTITY GENERATING PART
;;;                     BEGIN


;;; Context clause
;;;
;;; According to IEEE 1076-1993 11.3:
;;;
;;; context_clause := { context_item }
;;; context_item := library_clause | use_clause
;;;
;;; Implementation note:
;;;    Both library and use clauses will be generated, eventually...
;;;    What is missing is the information from gEDA itself, i think.


;;; writes some needed library insertions staticly
;;; not really clever, but a first solution

(define vams:write-context-clause
  (lambda ()
    (display "LIBRARY ieee,disciplines;\n")
    (display "USE ieee.math_real.all;\n")
    (display "USE ieee.math_real.all;\n")
    (display "USE work.electrical_system.all;\n")
    (display "USE work.all;\n")))



;;; Primary unit
;;;
;;; According to IEEE 1076-1993 11.1:
;;;
;;; primary_unit :=
;;;    entity_declaration
;;;  | configuration_declaration
;;;  | package_declaration
;;;
;;; Implementation note:
;;;    We assume that gEDA does not generate either a configuration or
;;;    package declaration. Thus, only a entity declaration will be generated.
;;;
;;; According to IEEE 1076-1993 1.1:
;;;
;;; entity_declaration :=
;;;    ENTITY identifier IS
;;;       entity_header
;;;       entity_declarative_part
;;;  [ BEGIN
;;;       entity_statement_part ]
;;;    END [ ENTITY ] [ entity_simple_name ] ;
;;;
;;; Implementation note:
;;;    We assume that no entity declarative part and no entity statement part
;;;    is to be produced. Further, it is good custom in VAMS-93 to append
;;;    both the entity keyword as well as the entity simple name to the
;;;    trailer, therefore this is done to keep VAMS compilers happy.
;;;
;;; According to IEEE 1076-1993 1.1.1:
;;;
;;; entity_header :=
;;;  [ formal_generic_clause ]
;;;  [ formal_port_clause ]
;;;
;;; Implementation note:
;;;    Initially we will assume that there is no generic clause but that there
;;;    is an port clause. We would very much like to have generic and the port
;;;    clause should be conditional (consider writting a test-bench).


;;; this routine managed the complete entity-declaration of a component
;;; or a schematic. It requires the entity-name, all ports and generics
;;; of this entity and the output-port. the output-port defines where
;;; this all should wrote to.

(define vams:write-primary-unit
  (lambda (entity port-list generic-list)
    (begin
      (vams:write-context-clause)
      (display "-- Entity declaration -- \n\n")
      (display "ENTITY ")
      (display entity)
      (display " IS\n")
      (vams:write-generic-clause generic-list)
      (vams:write-port-clause port-list)
      (display "END ENTITY ")
      (display entity)
      (display "; \n\n"))))



;;; GENERIC & PORT Clause
;;;
;;; According to IEEE 1076-1993 1.1.1:
;;;
;;; entity_header :=
;;;  [ formal_generic_clause ]
;;;  [ formal_port_clause ]
;;;
;;; generic_clause :=
;;;    GENERIC ( generic_list ) ;
;;;
;;; port_clause :=
;;;    PORT ( port_list ) ;
;;;
;;; According to IEEE 1076-1993 1.1.1.2:
;;;
;;; port_list := port_interface_list
;;;
;;; According to IEEE 1076-1993 4.3.2.1:
;;;
;;; interface_list := interface_element { ; interface_element }
;;;
;;; interface_element := interface_declaration
;;;
;;; According to IEEE 1076-1993 4.3.2:
;;;
;;; interface_declaration :=
;;;    interface_constant_declaration
;;;  | interface_signal_declaration
;;;  | interface_variable_declaration
;;;  | interface_file_declaration
;;;
;;; interface_signal_declaration :=
;;;  [ SIGNAL ] identifier_list : [ mode ] subtype_indication [ BUS ]
;;;  [ := static_expression ]
;;;
;;; mode := IN | OUT | INOUT | BUFFER | LINKAGE
;;;
;;; Implementation note:
;;;    Since the port list must contain signals will only the interface
;;;    signal declaration of the interface declaration be valid. Further,
;;;    we may safely assume that the SIGNAL symbol will not be needed.
;;;    The identifier list is reduced to a signle name entry, mode is set
;;;    to in, out or inout due to which part of the port list it comes from.
;;;    The mode types supported are in, out and inout where as buffer and
;;;    linkage mode is not supported. The subtype indication is currently
;;;    hardwired to standard logic, but should be controlled by attribute.
;;;    There is currently no support for busses and thus is the BUS symbol
;;;    no being applied. Also, there is currently no static expression
;;;    support, this too may be conveyed using attributes.


;;; this next two functions are writing the generic-clause
;;; in the entity declaration
;;; vams:write-generic-clause requires a list of all generics and
;;; its values, such like ((power 12.2) (velocity 233.34))

(define vams:write-generic-clause
  (lambda (generic-list)
    (if (not (null? generic-list))
        (begin
          (display "\t GENERIC (")
          (display "\t")
          (if (= 2 (length (car generic-list)))
              (begin
                (display (caar generic-list))
                (display " : REAL := ")
                (display (cadar generic-list))))
          (vams:write-generic-list (cdr generic-list))
          (display " );\n")))))

(define vams:write-generic-list
  (lambda (generic-list)
    (if (not (null? generic-list))
        (begin
          (display ";\n\t\t\t")
          (if (= 2 (length (car generic-list)))
              (begin
                (display (caar generic-list))
                (display " : REAL := ")
                (display (cadar generic-list))))
          (vams:write-generic-list (cdr generic-list))))))


;;; this function writes the port-clause in the entity-declarartion
;;; It requires a list of ports. ports stand for a list of all
;;; pin-attributes.

(define vams:write-port-clause
  (lambda (port-list)
    (if (not (null? port-list))
        (begin
          (display "\t PORT (\t")
          (display "\t")
          (if (list? (car port-list))
              (begin
                (display (cadar port-list))
                (display " \t")
                (display (caar port-list))
                (display " \t: ")
                (if (equal? (cadar port-list) 'quantity)
                    (display (car (cdddar port-list))))
                (display " \t")
                (display (caddar port-list))))
          (vams:write-port-list (cdr port-list))
          (display " );\n")))))

;;; This little routine writes a single pin on the port-clause.
;;; It requires a list containing (port_name, port_object, port_type, port_mode)
;;; such like
;;; ((heat quantity thermal in) (base terminal electrical unknown) .. )

(define vams:write-port-list
  (lambda (port-list)
    (if (not (null? port-list))
        (begin
          (display ";\n\t\t\t")
          (if (equal? (length (car port-list)) 4)
              (begin
                (display (cadar port-list))
                (display " \t")
                (display (caar port-list))
                (display " \t: ")
                (if (equal? (cadar port-list) 'quantity)
                    (display (car (cdddar port-list))))
                (display " \t")
                (display (caddar port-list))))
          (vams:write-port-list (cdr port-list))))))



;;;              ENTITY GENERATING PART
;;;                     END

;;; ===================================================================================

;;;           ARCHITECTURE GENERATING PART
;;;                   BEGIN



;; Secondary Unit Section
;;

;;; Architecture Declarative Part
;;;
;;; According to IEEE 1076-1993 1.2.1:
;;;
;;; architecture_declarative_part :=
;;;  { block_declarative_item }
;;;
;;; block_declarative_item :=
;;;    subprogram_declaration
;;;  | subprogram_body
;;;  | type_declaration
;;;  | subtype_declaration
;;;  | constant_declaration
;;;  | signal_declaration
;;;  | shared_variable_declaration
;;;  | file_declaration
;;;  | alias_declaration
;;;  | component_declaration
;;;  | attribute_declaration
;;;  | attribute_specification
;;;  | configuration_specification
;;;  | disconnection_specification
;;;  | use_clause
;;;  | group_template_declaration
;;;  | group_declaration
;;;
;;; Implementation note:
;;;    There is currently no support for programs or procedural handling in
;;;    gEDA, thus will all declarations above involved in thus activites be
;;;    left unused. This applies to subprogram declaration, subprogram body,
;;;    shared variable declaration and file declaration.
;;;
;;;    Further, there is currently no support for type handling and therefore
;;;    will not the type declaration and subtype declaration be used.
;;;
;;;    The is currently no support for constants, aliases, configuration
;;;    and groups so the constant declaration, alias declaration, configuration
;;;    specification, group template declaration and group declaration will not
;;;    be used.
;;;
;;;    The attribute passing from a gEDA netlist into VAMS attributes must
;;;    wait, therefore will the attribute declaration and attribute
;;;    specification not be used.
;;;
;;;    The disconnection specification will not be used.
;;;
;;;    The use clause will not be used since we pass the responsibility to the
;;;    primary unit (where it �s not yet supported).
;;;
;;;    The signal declation will be used to convey signals held within the
;;;    architecture.
;;;
;;;    The component declaration will be used to convey the declarations of
;;;    any external entity being used within the architecture.


;;; toplevel-subfunction for architecture generation.
;;; requires architecture and entity name and the port, where
;;; the architecture should wrote to.

(define vams:write-secondary-unit
  (lambda (architecture entity)
    (display "-- Secondary unit\n\n")
    (display "ARCHITECTURE ")
    (display architecture)
    (display " OF ")
    (display entity)
    (display " IS\n")
    (vams:write-architecture-declarative-part)
    (display "BEGIN\n")
    (vams:write-architecture-statement-part packages)
    (display "END ARCHITECTURE ")
    (display architecture)
    (display ";\n")))


;;;
;;; at this time, it only calls the signal declarations

(define vams:write-architecture-declarative-part
  (lambda ()
    (begin
      ; Due to my taste will the component declarations go first
      ; XXX - Broken until someday
      ; (vams:write-component-declarations packages)
      ; Then comes the signal declatations
      (vams:write-signal-declarations))))


;;; Signal Declaration
;;;
;;; According to IEEE 1076-1993 4.3.1.2:
;;;
;;; signal_declaration :=
;;;    SIGNAL identifier_list : subtype_indication [ signal_kind ]
;;;    [ := expression ] ;
;;;
;;; signal_kind := REGISTER | BUS
;;;
;;; Implementation note:
;;;    Currently will the identifier list be reduced to a single entry.
;;;    There is no support for either register or bus type of signal kind.
;;;    Further, no default expression is being supported.
;;;    The subtype indication is hardwired to Std_Logic.


;;; the really signal-declaration-writing function
;;; it's something more complex, because it's checking all signals
;;; for consistence. it only needs the output-port as parameter.

(define vams:write-signal-declarations
  (lambda ()
    (begin
      (for-each
       (lambda (net)
         (let*((connlist (gnetlist:get-all-connections net))
               (port_object (vams:net-consistence "port_object" connlist))
               (port_type (vams:net-consistence "port_type" connlist))
               )
           (if (and port_object
                    port_type
                    (if (equal? port_object "quantity")
                        (port_mode (vams:net-consistence 'port_mode connlist))))
               (begin
                 (display "\t")
                 (display port_object)
                 (display " ")
                 (display net)
                 (display " \t: ")
                 (display " ")
                 (display port_type)
                 (display ";\n"))
               (begin
                 (display "-- error in subnet : ")
                 (display net)
                 (newline)))))
       (vams:all-necessary-nets)))))


;;; Architecture Statement Part
;;;
;;; According to IEEE 1076-1993 1.2.2:
;;;
;;; architecture_statement_part :=
;;;  { concurrent_statement }
;;;
;;; According to IEEE 1076-1993 9:
;;;
;;; concurrent_statement :=
;;;    block_statement
;;;  | process_statement
;;;  | concurrent_procedure_call_statement
;;;  | concurrent_assertion_statement
;;;  | concurrent_signal_assignment_statement
;;;  | component_instantiation_statement
;;;  | generate_statement
;;;
;;; Implementation note:
;;;    We currently does not support block statements, process statements,
;;;    concurrent procedure call statements, concurrent assertion statements,
;;;    concurrent signal assignment statements or generarte statements.
;;;
;;;    Thus, we only support component instantiation statements.
;;;
;;; According to IEEE 1076-1993 9.6:
;;;
;;; component_instantiation_statement :=
;;;    instantiation_label : instantiation_unit
;;;  [ generic_map_aspect ] [ port_map_aspect ] ;
;;;
;;; instantiated_unit :=
;;;    [ COMPONENT ] component_name
;;;  | ENTITY entity_name [ ( architecture_identifier ) ]
;;;  | CONFIGURATION configuration_name
;;;
;;; Implementation note:
;;;    Since we are not supporting the generic parameters we will thus not
;;;    suppport the generic map aspect. We will support the port map aspect.
;;;
;;;    Since we do not yeat support the component form we will not yet use
;;;    the component symbol based instantiated unit.
;;;
;;;    Since we do not yeat support configurations we will not support the
;;;    we will not support the configuration symbol based form.
;;;
;;;    This leaves us with the entity form, which we will support initially
;;;    using only the entity name. The architecture identifier could possibly
;;;    be supported by attribute value.

;;; Component Declaration
;;;
;;; According to IEEE 1076-1993 4.5:
;;;
;;; component_declaration :=
;;;    COMPONENT identifier [ IS ]
;;;     [ local_generic_clause ]
;;;     [ local_port_clause ]
;;;    END COMPONENT [ component_simple_name ] ;
;;;
;;; Implementation note:
;;;    The component declaration should match the entity declaration of the
;;;    same name as the component identifier indicates. Since we do not yeat
;;;    support the generic clause in the entity declaration we shall not
;;;    support it here either. We will however support the port clause.
;;;
;;;    In the same fassion as before we will use the conditional IS symbol
;;;    as well as replicating the identifier as component simple name just to
;;;    be in line with good VAMS-93 practice and keep compilers happy.

;;; writes the architecture body.
;;; required all used packages, which are necessary for netlist-
;;; generation, and the output-port.

(define vams:write-architecture-statement-part
  (lambda (packages)
    (begin
      (display "-- Architecture statement part")
      (newline)
      (for-each (lambda (package)
                  (begin
                    (let ((device (get-device package))
                          (architecture
                           (gnetlist:get-package-attribute
                            package
                            "architecture")))
                      (if (not (memv (string->symbol device)
                                     (map string->symbol
                                          (list "IOPAD" "IPAD" "OPAD" "HIGH" "LOW"))))
                          (begin
                            (display " \n  ")

                            ;; writes instance-label
                            (display package)
                            (display " : ENTITY ")

                            ;; writes entity name, which should instanciated
                            (display (get-device package))

                            ;; write the architecture of an entity in brackets after
                            ;; the entity, when necessary.
                            (if (not (equal? architecture "unknown"))
                                (begin
                                  (display "(")
                                  (if (equal?
                                       (string-ref
                                        (gnetlist:get-package-attribute package
                                                                        "architecture") 0)
                                       #\?)
                                      (display (substring architecture 1))
                                      (display architecture))
                                  (display ")")))
                            (newline)

                            ;; writes generic map
                            (vams:write-generic-map package)

                            ;; writes port map
                            (vams:write-port-map package)

                            (display ";\n"))))))
                (vams:all-necessary-packages)))))



;; Given a uref, prints all generics attribute => values, without some
;; special attribs, like uref,source and architecture.
;; Don't ask why .... it's not the right place to discuss this.
;; requires the output-port and a uref

(define vams:write-generic-map
  (lambda (uref)
    (let ((new-ls (vams:all-used-generics
                   (vams:list-without-str-attrib
                    (vams:list-without-str-attrib
                     (vams:list-without-str-attrib
                      (gnetlist:vams-get-package-attributes uref)
                      "refdes") "source") "architecture") uref)))
      (if (not (null? new-ls))
          (begin
            (display "\tGENERIC MAP (\n")
            (vams:write-component-attributes uref new-ls)
            (display ")\n"))))))



;;; Port map aspect
;;;
;;; According to IEEE 1076-1993 5.6.1.2:
;;;
;;; port_map_aspect := PORT MAP ( port_association_list )
;;;
;;; According to IEEE 1076-1993 4.3.2.2:
;;;
;;; association_list :=
;;;    association_element { , association_element }

;;; writes the port map of the component.
;;; required output-port and uref.

(define vams:write-port-map
  (lambda (uref)
    (begin
      (let ((pin-list (gnetlist:get-pins-nets uref)))
        (if (not (null? pin-list))
            (begin
              (display "\tPORT MAP (\t")
              (vams:write-association-element (car pin-list))
              (for-each (lambda (pin)
                          (display ",\n")
                          (display "\t\t\t")
                          (vams:write-association-element pin))
                        (cdr pin-list))
              (display ")")))))))


;;; Association element
;;;
;;; According to IEEE 1076-1993 4.3.2.2:
;;;
;;; association_element :=
;;;  [ formal_part => ] actual_part
;;;
;;; formal_part :=
;;;    formal_designator
;;;  | function_name ( formal_designator )
;;;  | type_mark ( formal_designator )
;;;
;;; formal_designator :=
;;;    generic_name
;;;  | port_name
;;;  | parameter_name
;;;
;;; actual_part :=
;;;    actual_designator
;;;  | function_name ( actual_designator )
;;;  | type_mark ( actual_designator )
;;;
;;; actual_designator :=
;;;    expression
;;;  | signal_name
;;;  | variable_name
;;;  | file_name
;;;  | OPEN
;;;
;;; Implementation note:
;;;    In the association element one may have a formal part or relly on
;;;    positional association. The later is doomed out as bad VAMS practice
;;;    and thus will the formal part allways be present.
;;;
;;;    The formal part will not support either the function name or type mark
;;;    based forms, thus only the formal designator form is supported.
;;;
;;;    Of the formal designator forms will generic name and port name be used
;;;    as appropriate (this currently means that only port name will be used).
;;;
;;;    The actual part will not support either the function name or type mark
;;;    based forms, thus only the actual designator form is supported.


;;; the purpose of this function is very easy: write OPEN if pin
;;; unconnected and normal output if it connected.

(define vams:write-association-element
  (lambda (pin)
    (begin
      (display (car pin))
      (display " => ")
      (if (strncmp? (cdr pin) "unconnected_pin" 15)
          (display "OPEN")
          (display (vams:port-test pin))))))



;;; writes all generics of a component into the
;;; generic map. needs components uref, the generic-list and
;;; an output-port

(define vams:write-component-attributes
 (lambda (uref generic-list)
   (if (not (null? generic-list))
       (let ((attrib (car generic-list))
             (value (gnetlist:get-package-attribute uref (car generic-list))))
         (begin

           (if (string=? value "unknown")
             (vams:write-component-attributes uref (cdr generic-list))
             (begin
               (display "\t\t\t")
               (display attrib)
               (display " => ")
               (display value)
               (vams:write-component-attributes-helper uref (cdr generic-list)))))))))

(define vams:write-component-attributes-helper
 (lambda (uref generic-list)
   (if (not (null? generic-list))
       (let ((attrib (car generic-list))
             (value (gnetlist:get-package-attribute uref (car generic-list))))
         (begin

           (if (not (string=? value "unknown"))
             (begin
               (display ", ")
               (newline)
               (display "\t\t\t")
               (display attrib)
               (display " => ")
               (display value)
               (vams:write-component-attributes-helper uref (cdr generic-list)))))))))


;;;           ARCHITECTURE GENERATING PART
;;;                       END

;;; ===================================================================================

;;;
;;;           REALLY IMPORTANT HELP FUNCTIONS


;;; returns a list, whitout the specified string.
;;; requires: a list and a string

(define vams:list-without-str-attrib
  (lambda (ls str)
    (cond ((null? ls) '())
          (else
           (append
            (cond ((string=? (car ls) str) '())
                  (else (list (car ls))))
            (vams:list-without-str-attrib (cdr ls) str))))))



;; returns all not default-setted generics
;; After our definitions, all attribs, which values not started with a
;; '?' - character.

(define vams:all-used-generics
  (lambda (ls uref)
    (begin
      (if (null? ls)
          '()
          (append
           (if (equal? (string-ref (gnetlist:get-package-attribute uref (car ls)) 0) #\?)
               '()
               (list (car ls)))
           (vams:all-used-generics (cdr ls) uref))))))



;; checks all pins of a net for consistence, under different points
;; of view (pin-attributes).
;; requires: a pin-attribute and the subnet

(define vams:net-consistence
  (lambda (attribute connlist)
    (begin
      (if (equal? connlist '())
          #f
          (if (= (length connlist) 1)
              (if (equal? attribute 'port_mode)
                  (if (equal? (gnetlist:get-attribute-by-pinnumber (car (car connlist))
                                                          (car (cdr (car connlist)))
                                                          attribute)
                              'out)
                      #t
                      #f)
                  (append (gnetlist:get-attribute-by-pinnumber (car (car connlist))
                                                      (car (cdr (car connlist)))
                                                      attribute)))
              (if (equal? attribute 'port_mode)
                  (if (equal? (gnetlist:get-attribute-by-pinnumber (car (car connlist))
                                                          (car (cdr (car connlist)))
                                                          attribute)
                              'out)
                      #t
                      (vams:net-consistence attribute (cdr connlist)))
                  (if (equal? (gnetlist:get-attribute-by-pinnumber (car (car connlist))
                                                          (car (cdr (car connlist)))
                                                          attribute)
                              (vams:net-consistence attribute (cdr connlist)))
                      (append (gnetlist:get-attribute-by-pinnumber (car (car connlist))
                                                          (car (cdr (car connlist)))
                                                          attribute))
                      #f)))))))



;; returns a string, where are all whitespaces replaced to underlines
;; requires: a string only

(define vams:change-all-whitespaces-to-underlines
  (lambda (str)
    (begin
      (if (string-index str #\ )
          (begin
            (if (= (string-index str #\ ) (- (string-length str) 1))
                (vams:change-all-whitespaces-to-underlines
                 (substring str 0 (- (string-length str) 1)))
                (begin
                  (string-set! str (string-index str #\ ) #\_ )
                  (vams:change-all-whitespaces-to-underlines str))))
          (append str)))))



;; returns all nets, which a given list of pins are conneted to.
;; requires: uref and its pins

(define vams:all-pins-nets
  (lambda (uref pins)
    (if (null? pins)
        '()
        (append (list (car (gnetlist:get-nets uref (car pins))))
                (vams:all-pins-nets uref (cdr pins))))))



;; returns all nets, which a given list of urefs are connetd to
;; requires: list of urefs :-)

(define vams:all-packages-nets
  (lambda (urefs)
    (if (null? urefs)
        '()
        (append
         (vams:all-pins-nets (car urefs)
                             (gnetlist:get-pins (car urefs)))
         (vams:all-packages-nets (cdr urefs))))))



;; returns all ports from a list of urefs.
;; important for hierachical netlists. in our definition ports are
;; special components, which device-attributes a setted to "PORT".
;; The port-attributes are saved on toplevel of this special component.
;; requires: list of urefs

(define vams:all-ports-in-list
  (lambda (urefs)
    (begin
      (if (null? urefs)
          '()
          (append
           (if (equal? "PORT" (get-device (car urefs)))
               (list (car urefs))
               '())
           (vams:all-ports-in-list (cdr urefs)))))))



;; returns all nets in the schematic, which not
;; directly connected to a port.

(define vams:all-necessary-nets
  (lambda ()
    (vams:only-different-nets all-unique-nets
                              (vams:all-packages-nets
                               (vams:all-ports-in-list packages)))))



;; returns all elements from ls, that are not in without-ls.
;; a simple list function.
(define (vams:only-different-nets ls without-ls)
  (lset-difference equal? ls without-ls))


;; sort all port-components out

(define vams:all-necessary-packages
  (lambda ()
    (vams:only-different-nets packages
                              (vams:all-ports-in-list packages))))



;; if pin connetected to a port (special component), then return port.
;; else return the net, which the pin is connetcted to.
;; requires: a pin only

(define vams:port-test
  (lambda (pin)
    (if (member (cdr pin)
                (vams:all-packages-nets (vams:all-ports-in-list packages)))
        (append (vams:which-port
                 pin
                 (vams:all-ports-in-list packages)))
        (append (cdr pin)))))



;; returns the port, when is in port-list, which the pin is connected to
;; requires: a pin and a port-list

(define vams:which-port
  (lambda (pin ports)
    (begin
       (if (null? ports)
           '()
           (if (equal? (cdr pin)
                       (car (gnetlist:get-nets
                        (car ports)
                        (car (gnetlist:get-pins (car ports))))))
               (append (car ports))
               (append
                (vams:which-port pin (cdr ports))))))))



;; generate generic list for generic clause
;;((generic value) (generic value) .. ())

(define vams:generate-generic-list
  (lambda (ls)
    (if (null? ls)
        '()
        (append
         (if (not (or (string-prefix=? "refdes=" (car ls))
                      (string-prefix=? "source=" (car ls))
                      (string-prefix=? "architecture=" (car ls))))
             (list
              (if (string-index (car ls) #\=)
                  (list
                   (substring (car ls) 0 (string-rindex (car ls) #\= 0))
                   (substring (car ls) (+ (string-rindex (car ls) #\= 0)
                                          (if (equal? (string-ref
                                                       (car ls)
                                                       (1+ (string-rindex (car ls) #\= 0)))
                                                       #\?)
                                              2 1))
                              (string-length (car ls))))
                  (car ls)))
             '())
         (vams:generate-generic-list (cdr ls))))))



;;; generates a port list of the current schematic, or returns
;;; a empty list, if no port reachable.

(define vams:generate-port-list
  (lambda (uref)
    (let ((port-list  (list '())))
      (if (null? uref)
          '()
          (begin
            (for-each (lambda (pin)
                        (append! port-list
                                 (list (list pin
                                             (gnetlist:get-attribute-by-pinnumber uref pin "port_object")
                                             (gnetlist:get-attribute-by-pinnumber uref pin "port_type")
                                             (gnetlist:get-attribute-by-pinnumber uref pin "port_mode")))))
                      (gnetlist:get-pins uref))
            (append (cdr port-list)))))))



;;; gets the uref value from the top-attribs-list, which is assigned from gschem.
;;; only important for automatic-gnetlist-calls from gschem !!!

(define vams:get-uref
  (lambda (liste)
    (begin
      (if (null? liste)
          '()
          (if (string-prefix=? "refdes=" (symbol->string (car liste)))
              (begin
                (append (substring (car liste) 5
                                   (string-length (car liste)))))
              (vams:get-uref (cdr liste)))))))


;;; set generate-mode to default (1), when not defined before.
(define generate-mode (if (defined? 'generate-mode) generate-mode '1))


;;; set to-attribs list empty, when not needed.
(define top-attribs (if (defined? 'top-attribs) top-attribs '()))

(message "loaded gnet-vams.scm\n")