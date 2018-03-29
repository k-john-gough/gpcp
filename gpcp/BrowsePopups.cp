(* ==================================================================== *)
(*									*)
(*  Utility Module for the Gardens Point Component Symbol File Browser  *)
(*	Copyright (c) John Gough 2018.                                  *)
(*      This module defines the CSS prefix and JavaScript suffix        *)
(*									*)
(* ==================================================================== *)

MODULE BrowsePopups;
  IMPORT RTS;

  CONST stylePrefix* =
!"<style>\n" +
!"<!-- BEGIN Inline CSS -->\n" +
!"/* Popup container */\n" +
!".popup {\n" +
!"    position: relative;\n" +
!"    display: inline-block;\n" +
!"    cursor: pointer;\n" +
!"}\n\n" +
!"/* The actual popup (appears on top) */\n" +
!".popup .popuptext {\n" +
!"    visibility: hidden;\n" +
!"    width: 430px;\n" +
!"    background-color: white;\n" +
!"    color: black;\n" +
!"    text-align: left;\n" +
!"    border-radius: 6px;\n" +
!"    border-style: solid;\n" +
!"    border-color: black;\n" +
!"    padding: 8px 8px 8px 8px;\n" +
!"    position: absolute;\n" +
!"    z-index: 1;\n" +
!"    margin-top: 30px;\n" +
!"}\n\n" + 
!"/* Toggle this class when clicking on the popup container (hide and show the popup) */\n" +
!".popup .show {\n" +
!"    visibility: visible;\n" +
!"    opacity: 1;\n" +
!"}\n\n" +
!"</style>\n" +
!"<!-- END Inline CSS -->";

  CONST ecmaScript* =
!"<!-- BEGIN JavaScript -->\n" +
!"<script>\n" +
!"function toggle(node) {\n" +
!"    node.classList.toggle(\"show\"); \n" +
!"  }\n\n" +
!"function cls(node) {\n" +
!"    var prnt = node.parentNode;\n" +
!"    var next = prnt.previousSibling;\n" +
!"    var text = next.lastChild;\n" +
!"    text.innerHTML = \n" +
!"        \"The <b>class</b> property asserts that this type will be<br>\" +\n" +
!"        \"implemented as a reference class in the underlying framework.\";\n" +
!"    text.classList.toggle(\"show\");\n" +
!"  }\n\n" +
!"function newOk(node) {\n" +
!"    var popuptext = node.parentNode.previousSibling.lastChild;\n" +
!"    popuptext.innerHTML = \n" +
!"        \"The <b>has noArg-ctor</b> property asserts that this reference<br>\" +\n" +
!"        \"class has a no-arg constructor, and thus objects of this<br>\" +\n" +
!"        \"type can be allocated with NEW(). Component Pascal types<br>\" +\n" +
!"        \"derived from this type can also be allocated using NEW().\";\n" +
!"    popuptext.classList.toggle(\"show\"); \n" +
!"  }\n\n" +
!"function noNew(node) { \n" +
!"    var prnt = node.parentNode; \n" +
!"    var next = prnt.previousSibling; \n" +
!"    var text = next.lastChild; \n" +
!"    text.innerHTML =  \n" +
!"        \"The <b>no noArg-ctor</b> property asserts that this reference<br>\" +\n" +
!"        \"class does not have a noArg constructor. Objects of this<br>\" +\n" +
!"        \"type cannot be allocated using NEW(), and Component<br>\" +\n" +
!"        \"Pascal types  derived from this type also cannot be<br>\" +\n" +
!"        \"allocated using NEW().\";\n" +
!"    text.classList.toggle(\"show\");\n" +
!"  }\n\n" +
!"function noCpy(node) {\n" +
!"    var popuptext = node.parentNode.previousSibling.lastChild;\n" +
!"    popuptext.innerHTML = \n" +
!"        \"The <b>no value-assign</b> property asserts that this reference<br>\" + \n" +
!"        \"class does not have a value copy method. Therefore, it<br>\" +\n" +
!"        \"is not possible to perform value assignments using<br>\" +\n" +
!"        \"statements like <i>dst^</i> := <i>src^</i>\";\n" +
!"    popuptext.classList.toggle(\"show\");\n" +
!"  }\n\n" +
!"function valueclass(node) { \n" +
!"        var popuptext = node.parentNode.previousSibling.lastChild; \n" +
!"    popuptext.innerHTML =  \n" +
!"        \"The <b>valueclass</b> property asserts that this RECORD type is<br>\" + \n" +
!"        \"implemented as a value class in the CLR. It follows that<br>\" +  \n" +
!"        \"value assignments can be made of these types. They can<br>\" + \n" +
!"        \"also be passed by value. Even pointers to records of<br>\" + \n" +
!"        \"such types may be dereferenced and copied using syntax<br>\" + \n" +
!"        \"such as  <i>dst^</i> := <i>src^</i>\"; \n" +
!"    popuptext.classList.toggle(\"show\"); \n" +
!"  } \n\n" +
!"function argCtor(node) { \n" +
!"    var popuptext = node.parentNode.previousSibling.lastChild; \n" +
!"    popuptext.innerHTML =  \n" +
!"        \"The <b>has arg-ctor</b> property asserts that this reference<br>\" +  \n" +
!"        \"class has one or more constructors that take arguments.<br>\" + \n" +
!"        \"These constructors can be called from Component Pascal<br>\" + \n" +
!"        \"by calling an <i>init</i>(...) method from the static list<br>\" + \n" +
!"        \"here. Component Pascal types derived from this type<br>\" + \n" +
!"        \"can define constructors with arguments which call the<br>\" + \n" +
!"        \"super-type constructor.<br>\" + \n" +
!"        \"See <i>'Interfacing to constructors'</i> in the Release Notes.\"; \n" +
!"    popuptext.classList.toggle(\"show\"); \n" +
!"  } \n\n" +
!"</script>\n" +
!"<!-- END JavaScript --> \n";

END BrowsePopups.
    
