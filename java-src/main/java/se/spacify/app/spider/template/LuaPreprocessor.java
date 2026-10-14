package se.spacify.app.spider.template;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Translates a Spider template into a runnable Lua chunk — the ASP/JSP-style
 * preprocessing pass of the {@link TemplateEngine}. The template mixes literal
 * markup with two directive forms:
 *
 * <ul>
 *   <li><b>{@code ${expr}}</b> — an interpolation: {@code expr} is evaluated as a
 *       Lua expression and its {@code tostring()} spliced into the output.</li>
 *   <li><b>{@code %}-prefixed lines</b> — control flow emitted as raw Lua. The
 *       loop shorthand {@code % for i,10 do} (count form) is rewritten to the Lua
 *       numeric loop {@code for i=1,10 do}; everything else (e.g. {@code % end},
 *       {@code % if x then}) passes through verbatim.</li>
 * </ul>
 *
 * <p>The emitted chunk accumulates markup into a buffer and {@code return}s the
 * concatenated string, so {@link TemplateEngine} can run it and parse the result
 * as XML. Example:
 *
 * <pre>{@code
 *   <text>${i}</text>
 *   % for i,10 do
 *   <text>${i}</text>
 *   % end
 * }</pre>
 *
 * becomes (roughly):
 *
 * <pre>{@code
 *   local __out = {}
 *   local function __emit(s) __out[#__out + 1] = s end
 *   __emit("<text>") __emit(__str(i)) __emit("</text>\n")
 *   for i=1,10 do
 *   __emit("<text>") __emit(__str(i)) __emit("</text>\n")
 *   end
 *   return table.concat(__out)
 * }</pre>
 */
public final class LuaPreprocessor {

    /** {@code % for <var>,<count> do} — the count-form loop shorthand from the spec. */
    private static final Pattern COUNT_FOR =
        Pattern.compile("^\\s*for\\s+([A-Za-z_]\\w*)\\s*,\\s*(.+?)\\s+do\\s*$");

    private LuaPreprocessor() {
    }

    /** Compile {@code template} into a self-contained Lua chunk that returns the rendered markup. */
    public static String compile(String template) {
        StringBuilder lua = new StringBuilder();
        lua.append("local __out = {}\n");
        lua.append("local function __emit(s) __out[#__out + 1] = s end\n");
        lua.append("local function __str(v) if v == nil then return \"\" end return tostring(v) end\n");

        // Process line by line so a leading '%' can be recognised as a code line.
        for (String line : template.split("\n", -1)) {
            String trimmed = line.stripLeading();
            if (trimmed.startsWith("%")) {
                lua.append(compileCodeLine(trimmed.substring(1))).append('\n');
            } else {
                lua.append(compileTextLine(line)).append('\n');
            }
        }

        lua.append("return table.concat(__out)\n");
        return lua.toString();
    }

    /** A {@code %} line: rewrite the count-form {@code for} shorthand, otherwise pass the Lua through. */
    private static String compileCodeLine(String code) {
        Matcher m = COUNT_FOR.matcher(code);
        if (m.matches()) {
            return "for " + m.group(1) + "=1," + m.group(2).trim() + " do";
        }
        return code.strip();
    }

    /** A markup line: split it into literal runs and {@code ${...}} interpolations, then emit each. */
    private static String compileTextLine(String line) {
        StringBuilder out = new StringBuilder();
        int i = 0;
        while (i < line.length()) {
            int open = line.indexOf("${", i);
            if (open < 0) {
                emitLiteral(out, line.substring(i));
                break;
            }
            int close = line.indexOf('}', open + 2);
            if (close < 0) {
                // Unterminated interpolation: treat the remainder as literal text.
                emitLiteral(out, line.substring(i));
                break;
            }
            emitLiteral(out, line.substring(i, open));
            String expr = line.substring(open + 2, close).trim();
            out.append(" __emit(__str(").append(expr).append("))");
            i = close + 1;
        }
        // Restore the line break the split() consumed so the XML keeps its shape.
        emitLiteral(out, "\n");
        return out.toString();
    }

    private static void emitLiteral(StringBuilder out, String text) {
        if (!text.isEmpty()) {
            out.append(" __emit(").append(quote(text)).append(')');
        }
    }

    /** Render {@code text} as a double-quoted Lua string literal. */
    private static String quote(String text) {
        StringBuilder sb = new StringBuilder(text.length() + 2);
        sb.append('"');
        for (int i = 0; i < text.length(); i++) {
            char c = text.charAt(i);
            switch (c) {
                case '\\' -> sb.append("\\\\");
                case '"'  -> sb.append("\\\"");
                case '\n' -> sb.append("\\n");
                case '\r' -> sb.append("\\r");
                case '\t' -> sb.append("\\t");
                default   -> sb.append(c);
            }
        }
        sb.append('"');
        return sb.toString();
    }
}
