package se.spacify.app.playlist.upsl;

import java.math.BigInteger;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;

/**
 * Base62 codec for UPSL leaves (RFC-0001 §4.1). The alphabet is index → glyph
 * {@code 0-9} (0–9), {@code A-Z} (10–35), {@code a-z} (36–61). A byte string is
 * treated as a big-endian unsigned integer and rendered in base 62; each leading
 * {@code 0x00} byte is preserved as a leading {@code '0'} glyph (as Base58Check
 * preserves leading zeros), so the transform is exactly reversible.
 *
 * <p>{@link #decodeText(String)} / {@link #encodeText(String)} apply the codec to
 * the UTF-8 bytes of a string, which is how UPSL carries user text (name,
 * description, content refs) past the structural {@code :}/{@code ,} delimiters.
 */
public final class Base62 {

    private static final String ALPHABET = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    private static final BigInteger BASE = BigInteger.valueOf(62);

    private Base62() {
    }

    /** {@code B62(UTF8(text))} — decode a Base62 leaf back to its UTF-8 string. */
    public static String decodeText(String encoded) {
        return new String(decode(encoded), StandardCharsets.UTF_8);
    }

    /** {@code B62(UTF8(text))} — encode a string's UTF-8 bytes as a Base62 leaf. */
    public static String encodeText(String text) {
        return encode(text.getBytes(StandardCharsets.UTF_8));
    }

    /**
     * Decode a Base62 string into the bytes it represents, preserving leading
     * {@code '0'} glyphs as leading zero bytes.
     *
     * @throws IllegalArgumentException if a character is outside the alphabet
     *         (a reader must reject leaves that don't cleanly decode — RFC-0001 §12).
     */
    public static byte[] decode(String encoded) {
        int zeros = 0;
        while (zeros < encoded.length() && encoded.charAt(zeros) == '0') {
            zeros++;
        }

        BigInteger value = BigInteger.ZERO;
        for (int i = zeros; i < encoded.length(); i++) {
            int digit = ALPHABET.indexOf(encoded.charAt(i));
            if (digit < 0) {
                throw new IllegalArgumentException("Not a Base62 character: '" + encoded.charAt(i) + "'");
            }
            value = value.multiply(BASE).add(BigInteger.valueOf(digit));
        }

        byte[] magnitude = toUnsignedBytes(value);
        byte[] out = new byte[zeros + magnitude.length];
        System.arraycopy(magnitude, 0, out, zeros, magnitude.length);
        return out;
    }

    /** Encode bytes as Base62, emitting one leading {@code '0'} glyph per leading zero byte. */
    public static String encode(byte[] bytes) {
        int zeros = 0;
        while (zeros < bytes.length && bytes[zeros] == 0) {
            zeros++;
        }

        StringBuilder sb = new StringBuilder();
        BigInteger value = new BigInteger(1, bytes);
        while (value.signum() > 0) {
            BigInteger[] divRem = value.divideAndRemainder(BASE);
            sb.append(ALPHABET.charAt(divRem[1].intValue()));
            value = divRem[0];
        }
        for (int i = 0; i < zeros; i++) {
            sb.append('0');
        }
        return sb.reverse().toString();
    }

    /** {@code value} as its minimal big-endian unsigned byte array (no sign byte). */
    private static byte[] toUnsignedBytes(BigInteger value) {
        if (value.signum() == 0) {
            return new byte[0];
        }
        byte[] full = value.toByteArray();
        // BigInteger prepends a 0x00 sign byte when the high bit is set; drop it.
        return (full[0] == 0) ? Arrays.copyOfRange(full, 1, full.length) : full;
    }
}
