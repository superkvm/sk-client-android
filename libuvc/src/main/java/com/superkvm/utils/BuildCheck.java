package com.superkvm.utils;


import android.os.Build;

public final class BuildCheck {

	private static final boolean check(final int value) {
		return (Build.VERSION.SDK_INT >= value);
	}

	
	public static boolean isCurrentDevelopment() {
		return (Build.VERSION.SDK_INT == Build.VERSION_CODES.CUR_DEVELOPMENT);
	}

	
	public static boolean isBase() {
		return check(Build.VERSION_CODES.BASE);
	}

	
	public static boolean isBase11() {
		return check(Build.VERSION_CODES.BASE_1_1);
	}

	
	public static boolean isCupcake() {
		return check(Build.VERSION_CODES.CUPCAKE);
	}

	
	public static boolean isAndroid1_5() {
		return check(Build.VERSION_CODES.CUPCAKE);
	}

	
	public static boolean isDonut() {
		return check(Build.VERSION_CODES.DONUT);
	}

	
	public static boolean isAndroid1_6() {
		return check(Build.VERSION_CODES.DONUT);
	}

	
	public static boolean isEclair() {
		return check(Build.VERSION_CODES.ECLAIR);
	}

	
	public static boolean isAndroid2_0() {
		return check(Build.VERSION_CODES.ECLAIR);
	}

	
	public static boolean isEclair01() {
		return check(Build.VERSION_CODES.ECLAIR_0_1);
	}

	
	public static boolean isEclairMR1() {
		return check(Build.VERSION_CODES.ECLAIR_MR1);
	}

	
	public static boolean isFroyo() {
		return check(Build.VERSION_CODES.FROYO);
	}

	
	public static boolean isAndroid2_2() {
		return check(Build.VERSION_CODES.FROYO);
	}

	
	public static boolean isGingerBread() {
		return check(Build.VERSION_CODES.GINGERBREAD);
	}

	
	public static boolean isAndroid2_3() {
		return check(Build.VERSION_CODES.GINGERBREAD);
	}

	
	public static boolean isGingerBreadMR1() {
		return check(Build.VERSION_CODES.GINGERBREAD_MR1);
	}

	
	public static boolean isAndroid2_3_3() {
		return check(Build.VERSION_CODES.GINGERBREAD_MR1);
	}

	
	public static boolean isHoneyComb() {
		return check(Build.VERSION_CODES.HONEYCOMB);
	}

	
	public static boolean isAndroid3() {
		return check(Build.VERSION_CODES.HONEYCOMB);
	}

	
	public static boolean isHoneyCombMR1() {
		return check(Build.VERSION_CODES.HONEYCOMB_MR1);
	}

	
	public static boolean isAndroid3_1() {
		return check(Build.VERSION_CODES.HONEYCOMB_MR1);
	}

	
	public static boolean isHoneyCombMR2() {
		return check(Build.VERSION_CODES.HONEYCOMB_MR2);
	}

	
	public static boolean isAndroid3_2() {
		return check(Build.VERSION_CODES.HONEYCOMB_MR2);
	}

	
	public static boolean isIcecreamSandwich() {
		return check(Build.VERSION_CODES.ICE_CREAM_SANDWICH);
	}

	
	public static boolean isAndroid4() {
		return check(Build.VERSION_CODES.ICE_CREAM_SANDWICH);
	}

	
	public static boolean isIcecreamSandwichMR1() {
		return check(Build.VERSION_CODES.ICE_CREAM_SANDWICH_MR1);
	}

	
	public static boolean isAndroid4_0_3() {
		return check(Build.VERSION_CODES.ICE_CREAM_SANDWICH_MR1);
	}

	
	public static boolean isJellyBean() {
		return check(Build.VERSION_CODES.JELLY_BEAN);
	}

	
	public static boolean isAndroid4_1() {
		return check(Build.VERSION_CODES.JELLY_BEAN);
	}

	
	public static boolean isJellyBeanMr1() {
		return check(Build.VERSION_CODES.JELLY_BEAN_MR1);
	}

	
	public static boolean isAndroid4_2() {
		return check(Build.VERSION_CODES.JELLY_BEAN_MR1);
	}

	
	public static boolean isJellyBeanMR2() {
		return check(Build.VERSION_CODES.JELLY_BEAN_MR2);
	}

	
	public static boolean isAndroid4_3() {
		return check(Build.VERSION_CODES.JELLY_BEAN_MR2);
	}

	
	public static boolean isKitKat() {
		return check(Build.VERSION_CODES.KITKAT);
	}

	
	public static boolean isAndroid4_4() {
		return check(Build.VERSION_CODES.KITKAT);
	}

	
	public static boolean isKitKatWatch() {
		return (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT_WATCH);
	}

	
	public static boolean isL() {
		return (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP);
	}

	
	public static boolean isLollipop() {
		return (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP);
	}

	
	public static boolean isAndroid5() {
		return check(Build.VERSION_CODES.LOLLIPOP);
	}

	
	public static boolean isLollipopMR1() {
		return (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP_MR1);
	}

	
	public static boolean isM() {
		return check(Build.VERSION_CODES.M);
	}

	
	public static boolean isMarshmallow() {
		return check(Build.VERSION_CODES.M);
	}

	
	public static boolean isAndroid6() {
		return check(Build.VERSION_CODES.M);
	}

	
	public static boolean isN() {
		return check(Build.VERSION_CODES.N);
	}

	
	public static boolean isNougat() {
		return check(Build.VERSION_CODES.N);
	}
	
	public static boolean isAndroid7() {
		return check(Build.VERSION_CODES.N);
	}
	
	
	public static boolean isNMR1() {
		return check(Build.VERSION_CODES.N_MR1);
	}
	
	
	public static boolean isNougatMR1() {
		return check(Build.VERSION_CODES.N_MR1);
	}

	
	public static boolean isO() {
		return check(Build.VERSION_CODES.O);
	}
	
	
	public static boolean isOreo() {
		return check(Build.VERSION_CODES.O);
	}
	
	
	public static boolean isAndroid8() {
		return check(Build.VERSION_CODES.O);
	}
	
	
	public static boolean isOMR1() {
		return check(Build.VERSION_CODES.O_MR1);
	}

	
	public static boolean isOreoMR1() {
		return check((Build.VERSION_CODES.O_MR1));
	}
	
	
	public static boolean isP() {
		return check((Build.VERSION_CODES.P));
	}

	
	public static boolean isPie() {
		return check((Build.VERSION_CODES.P));
	}

	
	public static boolean isAndroid9() {
		return check((Build.VERSION_CODES.P));
	}
}
