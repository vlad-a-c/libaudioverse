{%import 'macros.t' as macros with context%}
import ctypes

libaudioverse_module = ctypes.cdll.LoadLibrary('libaudioverse.dll')

{%for name, val in constants.iteritems() -%}
{{name}} = {{val}}
{%endfor%}

{%for name, info in functions.iteritems()-%}
{{name}} = ctypes.CFUNCTYPE({{macros.ctypes_string(info.return_type)}}
{%-if info.args|length > 0%}, {%endif%}{#some functions don't have arguments; if it doesn't, we must avoid the first comma#}
{%-for arg in info.args-%}
{{macros.ctypes_string(arg.type)}}
{%-if not loop.last%}, {%endif-%}{#put in a comma and space if needed#}
{%-endfor-%}
)(('{{name}}', libaudioverse_module))
{%endfor%}