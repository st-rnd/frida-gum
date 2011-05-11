/*
 * Copyright (C) 2010 Ole André Vadla Ravnås <ole.andre.ravnas@tandberg.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "script-fixture.c"

TEST_LIST_BEGIN (script)
  SCRIPT_TESTENTRY (invalid_script_should_return_null)

#if 0
  SCRIPT_TESTENTRY (arg_assignment_replaces_argument)
  SCRIPT_TESTENTRY (multiple_send_calls_produce_gvariant)
  SCRIPT_TESTENTRY (narrow_format_string_can_be_sent)
  SCRIPT_TESTENTRY (wide_format_string_can_be_sent)
  SCRIPT_TESTENTRY (byte_array_can_be_sent)
  SCRIPT_TESTENTRY (guid_can_be_sent)
  SCRIPT_TESTENTRY (return_value_can_be_sent)
  SCRIPT_TESTENTRY (bottom_half_executed_on_leave)
  SCRIPT_TESTENTRY (empty_script_is_executable_in_both_point_cuts)
#endif
TEST_LIST_END ()

typedef struct _StringAndLengthArgs StringAndLengthArgs;
typedef struct _StringsAndLengthArgs StringsAndLengthArgs;
typedef struct _NarrowFormatStringArgs NarrowFormatStringArgs;
typedef struct _WideFormatStringArgs WideFormatStringArgs;
typedef struct _ByteArrayAndLengthArgs ByteArrayAndLengthArgs;
typedef struct _GuidArgs GuidArgs;

struct _StringAndLengthArgs {
  gunichar2 * text;
  guint length;
};

struct _StringsAndLengthArgs {
  gchar * text_narrow;
  gunichar2 * text_wide;
  guint length;
};

struct _NarrowFormatStringArgs {
  gchar * format;
  gchar * name;
  guint age;
};

struct _WideFormatStringArgs {
  gunichar2 * format;
  gunichar2 * name;
  guint age;
};

struct _ByteArrayAndLengthArgs {
  gint socket;
  gpointer buffer;
  gsize length;
};

static void store_message (GumScript * script, GVariant * msg,
    gpointer user_data);

static gchar * narrow_string_from_utf8 (const gchar * str_utf8);

SCRIPT_TESTCASE (invalid_script_should_return_null)
{
  GError * err = NULL;

  g_assert (gum_script_from_string ("'", NULL) == NULL);

  g_assert (gum_script_from_string ("'", &err) == NULL);
  g_assert (err != NULL);
  g_assert_cmpstr (err->message, ==,
      "Script(line 1): SyntaxError: Unexpected token ILLEGAL");
}

#if 0

SCRIPT_TESTCASE (arg_assignment_replaces_argument)
{
  const gchar * script_text =
    "var new_text = \"No, not me!\"\n"
    "arg0 = &new_text\n"
    "arg1 = len (new_text)\n";
  GumScript * script;
  GError * error = NULL;
  gunichar2 * previous_text;
  guint previous_length;
  StringAndLengthArgs args;
  gchar * new_text;

  script = gum_script_from_string (script_text, &error);
  g_assert (error == NULL);
  g_assert (script != NULL);

  previous_text = g_utf8_to_utf16 ("Hey you", -1, NULL, NULL, NULL);
  previous_length = 7;
  args.text = previous_text;
  args.length = previous_length;
  fixture->argument_list = &args;

  gum_script_execute (script, &fixture->invocation_context);

  g_assert_cmphex (GPOINTER_TO_SIZE (args.text), !=,
      GPOINTER_TO_SIZE (previous_text));
  g_assert_cmpuint (args.length, !=, previous_length);
  new_text = g_utf16_to_utf8 (args.text, -1, NULL, NULL, NULL);
  g_assert_cmpstr (new_text, ==, "No, not me!");
  g_free (new_text);
  g_assert_cmpuint (args.length, ==, 11);

  g_free (previous_text);

  g_object_unref (script);
}

SCRIPT_TESTCASE (multiple_send_calls_produce_gvariant)
{
  const gchar * script_text =
    "send_narrow_string (arg0)\n"
    "send_wide_string (arg1)\n"
    "send_int32 (arg2)\n";
  GumScript * script;
  GError * error = NULL;
  StringsAndLengthArgs args;
  GVariant * msg = NULL;
  gchar * msg_str_narrow, * msg_str_wide;
  gint32 msg_int;

  script = gum_script_from_string (script_text, &error);
  g_assert (error == NULL);
  g_assert (script != NULL);
  args.text_narrow = narrow_string_from_utf8 ("ÆØÅæøå");
  args.text_wide = g_utf8_to_utf16 ("ÆØÅæøå", -1, NULL, NULL, NULL);
  args.length = 42;
  fixture->argument_list = &args;

  gum_script_set_message_handler (script, store_message, &msg, NULL);
  gum_script_execute (script, &fixture->invocation_context);
  g_assert (msg != NULL);
  g_assert (g_variant_is_of_type (msg, G_VARIANT_TYPE ("(ssi)")));
  g_variant_get (msg, "(ssi)", &msg_str_narrow, &msg_str_wide, &msg_int);
  g_assert_cmpstr (msg_str_narrow, ==, "ÆØÅæøå");
  g_assert_cmpstr (msg_str_wide, ==, "ÆØÅæøå");
  g_assert_cmpint (msg_int, ==, 42);
  g_free (msg_str_narrow);
  g_free (msg_str_wide);

  g_variant_unref (msg);

  g_free (args.text_wide);
  g_free (args.text_narrow);
  g_object_unref (script);
}

SCRIPT_TESTCASE (narrow_format_string_can_be_sent)
{
  const gchar * script_text = "send_narrow_format_string (arg0)";
  GumScript * script;
  GError * error = NULL;
  NarrowFormatStringArgs args;
  GVariant * msg = NULL;
  gchar * msg_str;

  script = gum_script_from_string (script_text, &error);
  g_assert (error == NULL);
  g_assert (script != NULL);
  args.format =
    narrow_string_from_utf8 ("My name is %s and I æm %%%03d");
  args.name = narrow_string_from_utf8 ("Bøggvald");
  args.age = 7;
  fixture->argument_list = &args;

  gum_script_set_message_handler (script, store_message, &msg, NULL);
  gum_script_execute (script, &fixture->invocation_context);
  g_assert (msg != NULL);
  g_assert (g_variant_is_of_type (msg, G_VARIANT_TYPE ("(s)")));
  g_variant_get (msg, "(s)", &msg_str);
  g_assert_cmpstr (msg_str, ==, "My name is Bøggvald and I æm %007");
  g_free (msg_str);

  g_variant_unref (msg);

  g_free (args.name);
  g_free (args.format);
  g_object_unref (script);
}

SCRIPT_TESTCASE (wide_format_string_can_be_sent)
{
  const gchar * script_text = "send_wide_format_string (arg0)";
  GumScript * script;
  GError * error = NULL;
  WideFormatStringArgs args;
  GVariant * msg = NULL;
  gchar * msg_str;

  script = gum_script_from_string (script_text, &error);
  g_assert (error == NULL);
  g_assert (script != NULL);
  args.format =
      g_utf8_to_utf16 ("My name is %s and I æm %%%03d", -1, NULL, NULL, NULL);
  args.name =
      g_utf8_to_utf16 ("Bøggvald", -1, NULL, NULL, NULL);
  args.age = 7;
  fixture->argument_list = &args;

  gum_script_set_message_handler (script, store_message, &msg, NULL);
  gum_script_execute (script, &fixture->invocation_context);
  g_assert (msg != NULL);
  g_assert (g_variant_is_of_type (msg, G_VARIANT_TYPE ("(s)")));
  g_variant_get (msg, "(s)", &msg_str);
  g_assert_cmpstr (msg_str, ==, "My name is Bøggvald and I æm %007");
  g_free (msg_str);

  g_variant_unref (msg);

  g_free (args.name);
  g_free (args.format);
  g_object_unref (script);
}

SCRIPT_TESTCASE (byte_array_can_be_sent)
{
  const gchar * script_text = "send_byte_array (arg1, arg2)";
  GumScript * script;
  GError * error = NULL;
  ByteArrayAndLengthArgs args;
  guint8 bytes[5] = { 'H', 'E', 'L', 'L', 'O' };
  GVariant * msg = NULL;
  GVariantIter * iter;
  guint i;
  guint8 element;

  script = gum_script_from_string (script_text, &error);
  g_assert (error == NULL);
  g_assert (script != NULL);
  args.socket = 1337;
  args.buffer = bytes;
  args.length = sizeof (bytes);
  fixture->argument_list = &args;

  gum_script_set_message_handler (script, store_message, &msg, NULL);
  gum_script_execute (script, &fixture->invocation_context);
  g_assert (msg != NULL);
  g_assert (g_variant_is_of_type (msg, G_VARIANT_TYPE ("(ay)")));
  g_variant_get (msg, "(ay)", &iter);
  g_assert_cmpuint (g_variant_iter_n_children (iter), ==, 5);
  for (i = 0; g_variant_iter_loop (iter, "y", &element); i++)
  {
    g_assert_cmpuint (i, <, 5);
    g_assert (element == bytes[i]);
  }
  g_assert_cmpuint (i, ==, 5);
  g_variant_iter_free (iter);

  g_variant_unref (msg);

  g_object_unref (script);
}

SCRIPT_TESTCASE (guid_can_be_sent)
{
  const gchar * script_text = "send_guid (arg0)";
  GumScript * script;
  GError * error = NULL;
  GumGuid guid;
  GuidArgs args;
  GVariant * msg = NULL;
  gchar * guid_str;

  script = gum_script_from_string (script_text, &error);
  g_assert (error == NULL);
  g_assert (script != NULL);

  guid.data1 = 0x3F2504E0;
  guid.data2 = 0x4F89;
  guid.data3 = 0x11D3;
  guid.data4 = GUINT64_TO_BE (G_GUINT64_CONSTANT (0x9A0C0305E82C3301));
  args.guid = &guid;
  fixture->argument_list = &args;

  gum_script_set_message_handler (script, store_message, &msg, NULL);
  gum_script_execute (script, &fixture->invocation_context);
  g_assert (msg != NULL);
  g_assert (g_variant_is_of_type (msg, G_VARIANT_TYPE ("(s)")));
  g_variant_get (msg, "(s)", &guid_str);
  g_assert_cmpstr (guid_str, ==, "{3F2504E0-4F89-11D3-9A0C-0305E82C3301}");
  g_free (guid_str);

  g_variant_unref (msg);

  g_object_unref (script);
}

SCRIPT_TESTCASE (return_value_can_be_sent)
{
  const gchar * script_text = "send_narrow_string (retval)";
  GumScript * script;
  GError * error = NULL;
  GVariant * msg = NULL;
  gchar buf[] = "I was returned";
  gchar * msg_str;

  script = gum_script_from_string (script_text, &error);
  g_assert (error == NULL);
  g_assert (script != NULL);

  gum_script_set_message_handler (script, store_message, &msg, NULL);

  fixture->return_value = buf;
  gum_script_execute (script, &fixture->invocation_context);
  memset (buf, 0, sizeof (buf));

  g_assert (msg != NULL);
  g_assert (g_variant_is_of_type (msg, G_VARIANT_TYPE ("(s)")));
  g_variant_get (msg, "(s)", &msg_str);
  g_assert_cmpstr (msg_str, ==, "I was returned");
  g_free (msg_str);
  g_variant_unref (msg);

  g_object_unref (script);
}

SCRIPT_TESTCASE (bottom_half_executed_on_leave)
{
  const gchar * script_text =
      "send_narrow_string (arg0)\n"
      "---\n"
      "send_int32 (retval)";
  GumScript * script;
  GError * error = NULL;
  GVariant * msg = NULL;
  StringsAndLengthArgs args = { 0, };
  gchar * str;
  gint val;

  script = gum_script_from_string (script_text, &error);
  g_assert (error == NULL);
  g_assert (script != NULL);

  gum_script_set_message_handler (script, store_message, &msg, NULL);

  args.text_narrow = "Hey";
  fixture->argument_list = &args;
  gum_script_execute (script, &fixture->invocation_context);
  g_assert (msg != NULL);
  g_assert (g_variant_is_of_type (msg, G_VARIANT_TYPE ("(s)")));
  g_variant_get (msg, "(s)", &str);
  g_assert_cmpstr (str, ==, "Hey");
  g_free (str);
  g_variant_unref (msg);
  msg = NULL;

  fixture->point_cut = GUM_POINT_LEAVE;
  fixture->return_value = GSIZE_TO_POINTER ((gsize) -42);
  gum_script_execute (script, &fixture->invocation_context);
  g_assert (msg != NULL);
  g_assert (g_variant_is_of_type (msg, G_VARIANT_TYPE ("(i)")));
  g_variant_get (msg, "(i)", &val);
  g_assert_cmpint (val, ==, -42);
  g_variant_unref (msg);

  g_object_unref (script);
}

SCRIPT_TESTCASE (empty_script_is_executable_in_both_point_cuts)
{
  const gchar * script_text = "";
  GumScript * script;
  GError * error = NULL;

  script = gum_script_from_string (script_text, &error);
  g_assert (error == NULL);
  g_assert (script != NULL);

  gum_script_execute (script, &fixture->invocation_context);

  fixture->point_cut = GUM_POINT_LEAVE;
  gum_script_execute (script, &fixture->invocation_context);

  g_object_unref (script);
}

#endif

static void
store_message (GumScript * script,
               GVariant * msg,
               gpointer user_data)
{
  GVariant ** testcase_msg_ptr = (GVariant **) user_data;

  g_assert (*testcase_msg_ptr == NULL);
  *testcase_msg_ptr = g_variant_ref (msg);
}

static gchar *
narrow_string_from_utf8 (const gchar * str_utf8)
{
#ifdef G_OS_WIN32
  glong len;
  gunichar2 * str_utf16;
  gchar * str_narrow;

  str_utf16 = g_utf8_to_utf16 (str_utf8, -1, NULL, &len, NULL);
  str_narrow = (gchar *) g_malloc0 (len * 2);
  WideCharToMultiByte (CP_ACP, 0, (LPCWSTR) str_utf16, -1, str_narrow, len * 2,
      NULL, NULL);
  g_free (str_utf16);

  return str_narrow;
#else
  return g_strdup (str_utf8);
#endif
}
