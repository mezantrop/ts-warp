PORTNAME=	ts-warp
DISTVERSION=	1.4.0.20231102
CATEGORIES=	net
MASTER_SITES=	GH

MAINTAINER=	zmey20000@yahoo.com
COMMENT=	Transparent proxy server and traffic wrapper
WWW=		https://github.com/mezantrop/ts-warp

LICENSE=	BSD2CLAUSE
LICENSE_FILE=	${WRKSRC}/LICENSE

USES=		tar:tgz

USE_GITHUB=	yes
GH_ACCOUNT=	mezantrop
GH_TAGNAME=	9ba6d03

MAKE_ARGS+=	CC="${CC}"

do-install:
	${INSTALL_PROGRAM} ${WRKSRC}/ts-warp ${STAGEDIR}${PREFIX}/bin
	${INSTALL_PROGRAM} ${WRKSRC}/ts-pass ${STAGEDIR}${PREFIX}/bin
	${INSTALL_SCRIPT} ${WRKSRC}/ts-warp.sh ${STAGEDIR}${PREFIX}/etc/rc.d
	${INSTALL_MAN} ${WRKSRC}/man/ts-pass.1 ${STAGEDIR}${MAN1PREFIX}/man/man1
	${INSTALL_MAN} ${WRKSRC}/man/ts-warp.sh.1 ${STAGEDIR}${MAN1PREFIX}/man/man1
	${INSTALL_MAN} ${WRKSRC}/man/ts-warp.5 ${STAGEDIR}${MAN5PREFIX}/man/man5
	${INSTALL_MAN} ${WRKSRC}/man/ts-warp.8 ${STAGEDIR}${MAN8PREFIX}/man/man8


	${INSTALL_DATA} ${WRKSRC}/examples/ts-warp.ini ${STAGEDIR}${PREFIX}/etc
	${INSTALL_DATA} ${WRKSRC}/examples/ts-warp_pf_freebsd.conf ${STAGEDIR}${PREFIX}/etc

.include <bsd.port.mk>
